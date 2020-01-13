**Requirements**
You need at least PHP 5.0.0 and pxlib >= 0.4.4 for the basic set of functions. Some newer functions are only available with pxlib >= 0.6.0. Reading and writing of encrypted databases requires at least pxlib >= 0.5.0. The paradox library (pxlib) is available at  [http://pxlib.sourceforge.net].

./configure
make
make install

[https://pecl.php.net/get/paradox-1.4.3.tgz]
change file paradox.c on paradox.c file from lib dir.

phpize
./configure
make
make install

/usr/local/lib/php
add to ini file extension=paradox.so

**Usage**
php p4u paradox2csv tmp_paradox_db/table_name.db
