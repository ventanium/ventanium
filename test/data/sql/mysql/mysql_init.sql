CREATE USER 'testuser'@'localhost' IDENTIFIED BY 'testpw';
CREATE DATABASE testdb;
GRANT ALL PRIVILEGES ON testdb.* TO testuser;
