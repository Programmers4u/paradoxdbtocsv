# Converter Pardox DB tables to CSV

## **Instalation**

git clone git@github.com:Programmers4u/paradoxdbtocsv.git

> docker-compose up

Login to shell docker container

> cd /usr/local/src

The paradox library (pxlib) is available at  [http://pxlib.sourceforge.net](http://pxlib.sourceforge.net).
download it and untar

> ./configure

> make

> make install

[https://pecl.php.net/get/paradox-1.4.3.tgz](https://pecl.php.net/get/paradox-1.4.3.tgz)
download it and utar

change file paradox.c on paradox.c file from *lib* dir.

> cp /var/www/lib/paradox.c /usr/local/src/paradox-1.4.3/paradox.c

> phpize

> ./configure

> make

> make install

add to ini file extension=paradox.so

> cp /var/www/docker-script/docker-php-ext-paradox_db.ini /usr/local/etc/php/conf.d/docker-php-ext-paradox_db.ini

**How to use?**

php p4u paradox2csv *path_to_paradox_table*

> php p4u paradox2csv tmp_paradox_db/table_name.db
