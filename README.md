# Ventanium 

Ventanium is a cross-platform general purpose C library with a focus on network
and database functionality.

## Features

* Abstract database interface inspired by JDBC and PDO makes it possible to
access different database systems via the same interface. Currently supported:
MySQL, MariaDB, SQLite
* Abstract network interface supporting IPv4, IPv6, UDP, TCP, TLS
* Provides an easy way to implement event-driven network servers with support
for `epoll(), kqueue(), select()`
* HTTP Server and Client with WebSocket support allows to implement WebAPIs in C
* NetworkMessaging Server and Client for UDP and TCP
* Thin wrappers around platform dependent things like threads, mutexes,
condition variables simplifies cross-platform development
* Many additional utils like configuration and log file handling

### Example: HTTP server

```c
#include <string.h>
#include <vtm/net/http/http_server.h>
#include <vtm/util/signal.h>

vtm_http_srv *srv;

void stop_server(int psig)
{
	vtm_signal_safe_puts("Stopping server...\n");
	vtm_http_srv_stop(srv);
}

void http_request(struct vtm_http_ctx *ctx, struct vtm_http_req *req, vtm_http_res *res)
{
	vtm_http_res_begin(res, VTM_HTTP_RES_MODE_FIXED, VTM_HTTP_200_OK);
	vtm_http_res_body_str(res, "Hello");
	vtm_http_res_end(res);
}

int main(void)
{
	int rc;
	struct vtm_http_srv_opts opts;

	/* setup SIGINT handler */
	vtm_signal_set_handler(VTM_SIG_INT, stop_server);

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* set options */
	memset(&opts, 0, sizeof(opts));
	opts.host = "127.0.0.1";
	opts.port = 5000;
	opts.backlog = 10;
	opts.events = 16;
	opts.threads = 4;
	opts.cbs.http_request = http_request;

	/* create http server */
	srv = vtm_http_srv_new();
	if (!srv) {
		vtm_err_print();
		goto end;
	}

	/* run */
	rc = vtm_http_srv_run(srv, &opts);
	if (rc != VTM_OK)
		vtm_err_print();

	/* free http server */
	vtm_http_srv_free(srv);

end:
	/* module cleanup */
	vtm_module_network_end();

	return 0;
}
```

### Example: SQL database access

```c
#include <vtm/sql/sqlite/sqlite.h>

int main(void)
{
	struct vtm_sql_module sql;
	struct vtm_sql_result result;
	vtm_sql_con *con;
	vtm_dataset *conf, *bind;
	size_t i;

	/* initialize sqlite connector */
	vtm_module_sqlite_init(&sql);

	/* database parameter */
	conf = vtm_dataset_new();
	vtm_dataset_set_string(conf, VTM_SQL_PAR_DATABASE, "test_db");

	/* open connection */
	con = sql.con_new(conf);

	/* create table */
	vtm_sql_execute(con, "CREATE TABLE IF NOT EXISTS users ("
			"user_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
			"cash INTEGER,"
			"name varchar(64) DEFAULT NULL)");

	/* create values */
	bind = vtm_dataset_new();
	for (i=0; i < 10; i++) {
		vtm_dataset_set_int(bind, "money", i);
		vtm_dataset_set_int(bind, "name", i*2);
		vtm_sql_execute_prepared(con, "INSERT INTO users(cash, name)"
			" VALUES(:money, :name)", bind);
	}

	/* query result */
	vtm_sql_query(con, "SELECT * FROM users WHERE cash > 5", &result);

	/* retrieve complete result set */
	vtm_sql_result_fetch_all(&result);

	/* print results */
	for (i=0; i < result.row_count; i++) {
		printf("Row %zu: user_id=%d cash=%d name=%s\n", i+1,
			vtm_dataset_get_int(&result.rows[i], "user_id"),
			vtm_dataset_get_int(&result.rows[i], "cash"),
			vtm_dataset_get_string(&result.rows[i], "name"));
	}
	vtm_sql_result_release(&result);

	/* release resources */
	vtm_dataset_free(bind);
	vtm_sql_con_free(con);
	vtm_dataset_free(conf);

	/* shutdown sql module */
	vtm_module_sql_thread_end(&sql);
	vtm_module_sql_end(&sql);

	return 0;
}
```

For the sake of clarity there is no error handling included in this example.

### Example: NetworkMessage UDP Server

```c
#include <string.h>
#include <vtm/core/error.h>
#include <vtm/net/nm/nm_dgram_server.h>
#include <vtm/util/signal.h>

vtm_nm_dgram_srv *srv;

void stop_server(int psig)
{
	vtm_signal_safe_puts("Stopping server...\n");
	vtm_nm_dgram_srv_stop(srv);
}

void server_ready(vtm_nm_dgram_srv *srv, struct vtm_nm_dgram_srv_opts *opts)
{
	printf("NM UDP Server listening at: %s:%u\n", opts->addr.host, opts->addr.port);
	fflush(stdout);
}

void msg_recv(vtm_nm_dgram_srv *srv, vtm_dataset *wd, vtm_dataset *msg, const struct vtm_socket_saddr *saddr)
{
	uint32_t a, b, c;

	/* calculate request */
	a = vtm_dataset_get_uint32(msg, "A");
	b = vtm_dataset_get_uint32(msg, "B");
	c = a + b;

	/* send response */
	vtm_dataset_clear(msg);
	vtm_dataset_set_uint32(msg, "C", c);
	vtm_nm_dgram_srv_send(srv, msg, saddr);
}

int main(void)
{
	int rc;
	struct vtm_nm_dgram_srv_opts opts;

	/* register signal handler */
	vtm_signal_set_handler(VTM_SIG_INT, stop_server);

	/* init network module */
	rc = vtm_module_network_init();
	if (rc != VTM_OK) {
		vtm_err_print();
		return EXIT_FAILURE;
	}

	/* create dgram server */
	srv = vtm_nm_dgram_srv_new();
	if (!srv) {
		vtm_err_print();
		goto end;
	}

	/* prepare options */
	memset(&opts, 0, sizeof(opts));
	opts.addr.family = VTM_SOCK_FAM_IN4;
	opts.addr.host = "127.0.0.1";
	opts.addr.port = 4000;
	opts.threads = 4;
	opts.cbs.server_ready = server_ready;
	opts.cbs.msg_recv = msg_recv;

	/* run server */
	rc = vtm_nm_dgram_srv_run(srv, &opts);
	if (rc != VTM_OK)
		vtm_err_print();

	/* free resources */
	vtm_nm_dgram_srv_free(srv);

end:
	/* shutdown network module */
	vtm_module_network_end();

	return 0;
}
```

### Other examples

For more information, see [Examples](/examples)

## Supported platforms

- Linux (tested on Debian)
- BSD (tested on FreeBSD)
- macOS / Darwin (tested on OS X 10.9.5)
- Windows with MinGW or Visual Studio 2017 (tested on Windows 10)

## Build

### Dependencies

Please install the required packages with the package manager of your operating
system. For simplicity this guide refers to Debian Linux with `apt-get` as
package manager.

#### Crypto

If you want to use cryptographic functionality such as TLS sockets you need to
install the OpenSSL development headers:

```
apt-get install libssl-dev

```

Otherwise you have to disable OpenSSL support with `NO_OPENSSL=1`.

#### MySQL

If you want to use the MySQL database interface you need to install
the MySQL C connector

```
apt-get install libmysqlclient-dev

```

or the MariaDB C connector as drop-in replacement

```
# Debian 8
apt-get install libmariadb-client-lgpl-dev libmariadb-client-lgpl-dev-compat

# Debian 9
apt-get install libmariadb-dev libmariadb-dev-compat
```

#### SQLite

For file based databases please install the SQLite3 development headers:

```
apt-get install libsqlite3-dev

```

### Building

The provided Makefile was written for the use with GNU Make. Use:

```
git clone https://ventanium.org/git/ventanium
cd ventanium
make
```

Some platforms use a different default make implementation, then you
can try

```
gmake
```

to build the library.

### Options

You can append the options described below to the make command. For example

```
make SYS=Windows NO_OPENSSL=1 DEBUG=1
```

would build the library for Windows without OpenSSL and with extra
debugging information.

You should use the same consistent set of options for building, testing and
installing  the library.

`SYS=<platform>`  
Override the auto detection of the Makefile. Possible
values are:

- Linux
- BSD
- Darwin
- Windows

`DEBUG=1`  
Include extra debugging information into the produced binaries.

`NO_OOM_ABORT=1`  
The library code aborts execution if a call to allocate new memory like
`malloc()` fails. With this flag this behavior can be supressed. 

`NO_OPENSSL=1`
Removes the dependency on the OpenSSL Library. Functions that rely on OpenSSL
will then return an error code like *VTM_E_NOT_SUPPORTED*.

`MYSQL=1`  
Build the library with the MySQL database interface.

`MARIADB=1`  
Build the library with the MySQL database interface, MariaDB is used as a
drop-in replacement for MySQL. This automatically enables `MYSQL=1`.

`SQLITE=1`  
Build the library with the SQLite3 database interface.

`DESTDIR=<path>`  
Specify the installation path for the headers and the compiled library.

`STATIC=1`  
Build statically linked example and test binaries.

### Test

To run the tests invoke

```
make runtest
```

After the tests have finished you should see the summary on your console:

```
-- Summary --
Total tests:       90
Tests passed:      90
Tests failed:       0
Test success rate is 100.00 %
Test finished!
```

### Examples

There a some code examples in the `examples` directory which can be build with:

```
make examples
```

The resulting binaries will be put into the `bin` directory.

### Installation

```
make install
```

The installation target also supports the **DESTDIR** and the **prefix**
variables to specify the installation path

```
make install DESTDIR=/tmp prefix=/opt
```

### Uninstall

There is also an uninstall target:

```
make uninstall
```

which removes all installed library and header files

### Linking

When linking your program against Ventanium you can get the necessary libraries
with the following command:

```
make ldinfo
```

## License

Ventanium is released under the 2-Clause BSD License,
see [LICENSE](LICENSE).

## Author
(C) 2018-2019 Matthias Benkendorf

## Contact

In case of problems, questions or any other remarks please contact me at
[lib@ventanium.org](lib@ventanium.org)
