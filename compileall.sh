#!/bin/bash
gcc -std=c99 -Wall keygen.c -o keygen
gcc -std=c99 -Wall enc_server.c -o enc_server
gcc -std=c99 -Wall enc_client.c -o enc_client
gcc -std=c99 -Wall dec_server.c -o dec_server
gcc -std=c99 -Wall dec_client.c -o dec_client