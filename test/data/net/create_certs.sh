#!/bin/bash
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 9125 -nodes -subj '/O=Ventanium/OU=Dev/CN=ventanium.test'
