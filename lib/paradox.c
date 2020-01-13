/*
  +----------------------------------------------------------------------+
  | PHP Version 4                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2003 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Uwe Steinmann <steinm@php.net>                               |
  +----------------------------------------------------------------------+
*/

/* $Id: paradox.c,v 1.37 2007/09/25 12:12:42 steinm Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"

#if HAVE_PARADOX

#include "php_paradox.h"

#include "zend_exceptions.h"

/* If you declare any globals in php_paradox.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(paradox)
*/

/* True global resources - no need for thread safety here */
static int le_pxdoc;

zend_class_entry *paradox_ce_db, *paradox_ce_exception;

static zend_object_handlers paradox_object_handlers_db;

/* {{{ paradox_functions[]
 *
 * Every user visible function must have an entry in paradox_functions[].
 */
zend_function_entry paradox_functions[] = {
	PHP_FE(px_new, NULL)
	PHP_FE(px_open_fp, NULL)
	PHP_FE(px_create_fp, NULL)
	PHP_FE(px_close, NULL)
	PHP_FE(px_delete, NULL)
	PHP_FE(px_numrecords, NULL)
	PHP_FE(px_numfields, NULL)
	PHP_FE(px_get_record, NULL)
	PHP_FE(px_put_record, NULL)
#ifdef HAVE_PXDELETERECORD
	PHP_FE(px_delete_record, NULL)
#endif
#ifdef HAVE_PXINSERTRECORD
	PHP_FE(px_insert_record, NULL)
#endif
#ifdef HAVE_PXUPDATERECORD
	PHP_FE(px_update_record, NULL)
#endif
#ifdef HAVE_PXRETRIEVERECORD
	PHP_FE(px_retrieve_record, NULL)
#endif
#ifdef HAVE_PXPACK
	PHP_FE(px_pack, NULL)
#endif
	PHP_FE(px_get_field, NULL)
	PHP_FE(px_get_schema, NULL)
	PHP_FE(px_get_info, NULL)
	PHP_FE(px_set_parameter, NULL)
	PHP_FE(px_get_parameter, NULL)
	PHP_FE(px_set_value, NULL)
	PHP_FE(px_get_value, NULL)
	PHP_FE(px_set_targetencoding, NULL)
	PHP_FE(px_set_tablename, NULL)
	PHP_FE(px_set_blob_file, NULL)
	PHP_FE(px_timestamp2string, NULL)
	PHP_FE(px_date2string, NULL)
	{NULL, NULL, NULL}	/* Must be the last line in paradox_functions[] */
};
/* }}} */

zend_function_entry paradox_funcs_db[] = {
	PHP_ME_MAPPING(__construct, px_new, NULL, 0)
	PHP_ME_MAPPING(open_fp, px_open_fp, NULL, 0)
	PHP_ME_MAPPING(create_fp, px_create_fp, NULL, 0)
	PHP_ME_MAPPING(close, px_close, NULL, 0)
	PHP_ME_MAPPING(numrecords, px_numrecords, NULL, 0)
	PHP_ME_MAPPING(numfields, px_numfields, NULL, 0)
	PHP_ME_MAPPING(get_record, px_get_record, NULL, 0)
	PHP_ME_MAPPING(put_record, px_put_record, NULL, 0)
#ifdef HAVE_PXDELETERECORD
	PHP_ME_MAPPING(delete_record, px_delete_record, NULL, 0)
#endif
#ifdef HAVE_PXINSERTRECORD
	PHP_ME_MAPPING(insert_record, px_insert_record, NULL, 0)
#endif
#ifdef HAVE_PXUPDATERECORD
	PHP_ME_MAPPING(update_record, px_update_record, NULL, 0)
#endif
#ifdef HAVE_PXRETRIEVERECORD
	PHP_ME_MAPPING(retrieve_record, px_retrieve_record, NULL, 0)
#endif
#ifdef HAVE_PXPACK
	PHP_ME_MAPPING(pack, px_pack, NULL, 0)
#endif
	PHP_ME_MAPPING(get_field, px_get_field, NULL, 0)
	PHP_ME_MAPPING(get_schema, px_get_schema, NULL, 0)
	PHP_ME_MAPPING(get_info, px_get_info, NULL, 0)
	PHP_ME_MAPPING(set_parameter, px_set_parameter, NULL, 0)
	PHP_ME_MAPPING(get_parameter, px_get_parameter, NULL, 0)
	PHP_ME_MAPPING(set_value, px_set_value, NULL, 0)
	PHP_ME_MAPPING(get_value, px_get_value, NULL, 0)
	PHP_ME_MAPPING(set_targetencoding, px_set_targetencoding, NULL, 0)
	PHP_ME_MAPPING(set_tablename, px_set_tablename, NULL, 0)
	PHP_ME_MAPPING(set_blob_file, px_set_blob_file, NULL, 0)
	PHP_ME_MAPPING(timestamp2string, px_timestamp2string, NULL, 0)
	PHP_ME_MAPPING(date2string, px_date2string, NULL, 0)
	{NULL, NULL, NULL}
};

/* {{{ paradox_module_entry
 */
zend_module_entry paradox_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"paradox",
	paradox_functions,
	PHP_MINIT(paradox),
	PHP_MSHUTDOWN(paradox),
	PHP_RINIT(paradox),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(paradox),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(paradox),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PARADOX
ZEND_GET_MODULE(paradox)
# ifdef PHP_WIN32
# include "zend_arg_defs.c"
# endif
#endif

/* {{{ php_paradox_px_dtor
 */
static ZEND_RSRC_DTOR_FUNC(php_paradox_px_dtor)
{
	if(rsrc->ptr) {
		pxdoc_t *pxdoc = (pxdoc_t *)rsrc->ptr;
		PX_delete(pxdoc);
		rsrc->ptr = NULL;
	}
}
/* }}} */

/* {{{ px_custom_errorhandler
 */
static void px_custom_errorhandler(pxdoc_t *p, int type, const char *shortmsg, void *data)
{
	TSRMLS_FETCH();
	switch(type) {
		case PX_MemoryError:
		case PX_IOError:
		case PX_RuntimeError:
			php_error_docref(NULL TSRMLS_CC, E_ERROR, shortmsg);
			break;
		case PX_Warning:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, shortmsg);
			break;
	}
}
/* }}} */

/* {{{ px_emalloc
 */
static void *px_emalloc(pxdoc_t *p, size_t size, const char *caller)
{
	void *ptr;
	ptr = emalloc(size);
#ifdef PARADOX_MEMDEBUG
	fprintf(stderr, "Malloc: 0x%X (%d Bytes): %s\n", ptr, size, caller);
#endif
	return(ptr);
}
/* }}} */

/* {{{ px_realloc
 */
static void *px_erealloc(pxdoc_t *p, void *mem, size_t size, const char *caller)
{
	return(erealloc(mem, size));
}
/* }}} */

/* {{{ px_efree
 */
static void px_efree(pxdoc_t *p, void *mem)
{
#ifdef PARADOX_MEMDEBUG
	fprintf(stderr, "Free: 0x%X\n", mem);
#endif
	efree(mem);
}
/* }}} */

/* {{{ px_estrdup
 */
static char* px_estrdup(pxdoc_t *p, const char *str) {
	char *tmp;
	int len;
	if(str == NULL)
		return(NULL);
	len = strlen(str)+1;
	if(NULL == (tmp = px_emalloc(p, len, "px_estrdup")))
		return(NULL);
	memcpy(tmp, str, len);
	return(tmp);
}
/* }}} */

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("paradox.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_paradox_globals, paradox_globals)
    STD_PHP_INI_ENTRY("paradox.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_paradox_globals, paradox_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_paradox_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_paradox_init_globals(zend_paradox_globals *paradox_globals)
{
	paradox_globals->global_value = 0;
	paradox_globals->global_string = NULL;
}
*/
/* }}} */

/* paradox_object_dtor() {{{
 */
static void paradox_object_dtor(void *object, zend_object_handle handle TSRMLS_DC) 
{
    paradox_object *intern = (paradox_object *)object;

    zend_hash_destroy(intern->zo.properties);
    FREE_HASHTABLE(intern->zo.properties);

    if (intern->ptr) {
		PX_delete(intern->ptr);
    }

    efree(object);
}
/* }}} */

/* paradox_object_new() {{{
 */
static void paradox_object_new(zend_class_entry *class_type, zend_object_handlers *handlers, zend_object_value *retval TSRMLS_DC)
{
    paradox_object *intern;
    zval *tmp;

    intern = emalloc(sizeof(paradox_object));
    memset(intern, 0, sizeof(paradox_object));
    intern->zo.ce = class_type;

    ALLOC_HASHTABLE(intern->zo.properties);
    zend_hash_init(intern->zo.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	object_properties_init((zend_object*) &(intern->zo), class_type);
	
	intern->ptr = PX_new2(px_custom_errorhandler, px_emalloc, px_erealloc, px_efree);
    retval->handle = zend_objects_store_put(intern, paradox_object_dtor, NULL, NULL TSRMLS_CC);
    retval->handlers = handlers;
}
/* }}} */

/* paradox_object_new_db() {{{
 */
static zend_object_value paradox_object_new_db(zend_class_entry *class_type TSRMLS_DC)
{
    zend_object_value retval;

    paradox_object_new(class_type, &paradox_object_handlers_db, &retval TSRMLS_CC);
    return retval;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(paradox)
{
	REGISTER_PARADOX_CLASS(db, NULL);

	/* If you have INI entries, uncomment these lines 
	ZEND_INIT_MODULE_GLOBALS(paradox, php_paradox_init_globals, NULL);
	REGISTER_INI_ENTRIES();
	*/
	le_pxdoc = zend_register_list_destructors_ex(php_paradox_px_dtor, NULL, "px object", module_number);

	/* File types */
	REGISTER_LONG_CONSTANT("PX_FILE_INDEX_DB", pxfFileTypIndexDB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FILE_PRIM_INDEX", pxfFileTypPrimIndex, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FILE_NON_INDEX_DB", pxfFileTypNonIndexDB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FILE_NON_INC_SEC_INDEX", pxfFileTypNonIncSecIndex, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FILE_SEC_INDEX", pxfFileTypSecIndex, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FILE_INC_SEC_INDEX", pxfFileTypIncSecIndex, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FILE_NON_INC_SEC_INDEX_G", pxfFileTypNonIncSecIndexG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FILE_SEC_INDEX_G", pxfFileTypSecIndexG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FILE_INC_SEC_INDEX_G", pxfFileTypIncSecIndexG, CONST_CS | CONST_PERSISTENT);

	/* Field types */
	REGISTER_LONG_CONSTANT("PX_FIELD_ALPHA", pxfAlpha, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_DATE", pxfDate, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_SHORT", pxfShort, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_LONG", pxfLong, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_CURRENCY", pxfCurrency, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_NUMBER", pxfNumber, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_LOGICAL", pxfLogical, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_MEMOBLOB", pxfMemoBLOb, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_BLOB", pxfBLOb, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_FMTMEMOBLOB", pxfFmtMemoBLOb, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_OLE", pxfOLE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_GRAPHIC", pxfGraphic, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_TIME", pxfTime, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_TIMESTAMP", pxfTimestamp, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_AUTOINC", pxfAutoInc, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_BCD", pxfBCD, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_FIELD_BYTES", pxfBytes, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_KEYTOLOWER", PX_KEYTOLOWER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PX_KEYTOUPPER", PX_KEYTOUPPER, CONST_CS | CONST_PERSISTENT);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(paradox)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(paradox)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(paradox)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(paradox)
{
	char tmp[32];

	snprintf(tmp, 31, "%d.%d.%d", PX_get_majorversion(), PX_get_minorversion(), PX_get_subminorversion() );
	php_info_print_table_start();
	php_info_print_table_row(2, "Paradox support", "enabled");
	php_info_print_table_row(2, "pxlib Version", tmp );
	switch(PX_has_recode_support()) {
		case 1: snprintf(tmp, 31, "recode"); break;
		case 2: snprintf(tmp, 31, "iconv"); break;
		default: snprintf(tmp, 31, "none"); break;
	}
	php_info_print_table_row(2, "support for recoding record data", tmp );
	php_info_print_table_row(2, "pxlib was build on", PX_get_builddate() );
	php_info_print_table_row(2, "Revision", "$Revision: 1.37 $" );
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ proto resource px_new()
   Creates a new Paradox file object */
PHP_FUNCTION(px_new)
{
	pxdoc_t *pxdoc;

	pxdoc = PX_new2(px_custom_errorhandler, px_emalloc, px_erealloc, px_efree);

	if(NULL == pxdoc) {
		RETURN_FALSE;
	}

	ZEND_REGISTER_RESOURCE(return_value, pxdoc, le_pxdoc);
}
/* }}} */

/* {{{ proto bool px_open_fp(resource pxdoc, resource filedesc)
   Opens an existing paradox document. */
PHP_FUNCTION(px_open_fp)
{
	zval *zpx, *zfp;
	FILE *fp = NULL;
	pxdoc_t *pxdoc = NULL;
	zval *object = getThis();
	php_stream *stream;

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zfp)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr", &zpx, &zfp)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	php_stream_from_zval(stream, &zfp);
	
	if (php_stream_cast(stream, PHP_STREAM_AS_STDIO, (void*)&fp, 1) == FAILURE)	{
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Stream could not be casted to stdio file.");
		RETURN_FALSE;
	}

	if (PX_open_fp(pxdoc, fp) < 0) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}

/* }}} */

/* {{{ proto bool px_create_fp(resource pxdoc, array fieldspec, resource filedesc)
   Creates a new paradox database. */
PHP_FUNCTION(px_create_fp)
{
	zval *zpx, *zfp;
	HashTable *lht;
	zval *schema, **keydataptr;
	FILE *fp = NULL;
	pxdoc_t *pxdoc = NULL;
	pxfield_t *pxf;
	int numfields, i;
	zval *object = getThis();
	php_stream *stream;

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz", &zfp, &schema)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rrz", &zpx, &zfp, &schema)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if (Z_TYPE_P(schema) != IS_ARRAY) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expected array as second parameter");
		RETURN_FALSE;
	}

	php_stream_from_zval(stream, &zfp);
	
	if (php_stream_cast(stream, PHP_STREAM_AS_STDIO, (void*)&fp, 1) == FAILURE)	{
		RETURN_FALSE;
	}

	lht = Z_ARRVAL_P(schema);
	if(0 == (numfields = zend_hash_num_elements(lht))) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "the column specification array is empty");
		RETURN_FALSE;
	}
	if(NULL == (pxf = (pxfield_t *)px_emalloc(pxdoc, numfields * sizeof(pxfield_t), "px_create_fp: Allocate memory for field specification."))) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not allocate memory for field specification.");
		RETURN_FALSE;
	}

	zend_hash_internal_pointer_reset(lht);
	for(i=0; i<numfields; i++) {
		HashTable *lht2;
		zval **value;
		char *str;
		zend_hash_get_current_data(lht, (void **) &keydataptr);
		if(Z_TYPE_PP(keydataptr) == IS_ARRAY) {
			lht2 = Z_ARRVAL_PP(keydataptr);

			/* field name */
			if (zend_hash_index_find(lht2, 0, (void **)&value) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "expected field name as first element of list in field %d", i);
				px_efree(pxdoc, pxf);
				RETURN_FALSE;
			}
			convert_to_string_ex(value);
			if (Z_STRLEN_PP(value) > 10 || Z_STRLEN_PP(value) == 0) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid field name '%s' (must be non-empty and less than or equal to 10 characters)", Z_STRVAL_PP(value));
				px_efree(pxdoc, pxf);
				RETURN_FALSE;
			}
			pxf[i].px_fname = px_estrdup(pxdoc, Z_STRVAL_PP(value));

			/* field type */
			if (zend_hash_index_find(lht2, 1, (void **)&value) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "expected field type as second element of list in field %d", i);
				px_efree(pxdoc, pxf[i].px_fname);
				px_efree(pxdoc, pxf);
				RETURN_FALSE;
			}
			convert_to_string_ex(value);
			str = Z_STRVAL_PP(value);
			/* verify the field length */
			switch ((int) str[0]) {
			case 'L':
				pxf[i].px_ftype = pxfLogical;
				pxf[i].px_flen = 1;
				break;
			case 'S':
				pxf[i].px_ftype = pxfShort;
				pxf[i].px_flen = 2;
				break;
			case 'I':
				pxf[i].px_ftype = pxfLong;
				pxf[i].px_flen = 4;
				break;
			case '+':
				pxf[i].px_ftype = pxfAutoInc;
				pxf[i].px_flen = 4;
				break;
			case 'M':
				pxf[i].px_ftype = pxfMemoBLOb;
				if (zend_hash_index_find(lht2, 2, (void **)&value) == FAILURE) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "expected field length as third element of list in field %d", i);
					px_efree(pxdoc, pxf[i].px_fname);
					px_efree(pxdoc, pxf);
					RETURN_FALSE;
				}
				convert_to_long_ex(value);
				pxf[i].px_flen = Z_LVAL_PP(value);
				break;
			case 'F':
				pxf[i].px_ftype = pxfFmtMemoBLOb;
				if (zend_hash_index_find(lht2, 2, (void **)&value) == FAILURE) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "expected field length as third element of list in field %d", i);
					px_efree(pxdoc, pxf[i].px_fname);
					px_efree(pxdoc, pxf);
					RETURN_FALSE;
				}
				convert_to_long_ex(value);
				pxf[i].px_flen = Z_LVAL_PP(value);
				break;
			case 'B':
				pxf[i].px_ftype = pxfBLOb;
				if (zend_hash_index_find(lht2, 2, (void **)&value) == FAILURE) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "expected field length as third element of list in field %d", i);
					px_efree(pxdoc, pxf[i].px_fname);
					px_efree(pxdoc, pxf);
					RETURN_FALSE;
				}
				convert_to_long_ex(value);
				pxf[i].px_flen = Z_LVAL_PP(value);
				break;
			case 'O':
				pxf[i].px_ftype = pxfOLE;
				if (zend_hash_index_find(lht2, 2, (void **)&value) == FAILURE) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "expected field length as third element of list in field %d", i);
					px_efree(pxdoc, pxf[i].px_fname);
					px_efree(pxdoc, pxf);
					RETURN_FALSE;
				}
				convert_to_long_ex(value);
				pxf[i].px_flen = Z_LVAL_PP(value);
				break;
			case 'G':
				pxf[i].px_ftype = pxfGraphic;
				if (zend_hash_index_find(lht2, 2, (void **)&value) == FAILURE) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "expected field length as third element of list in field %d", i);
					px_efree(pxdoc, pxf[i].px_fname);
					px_efree(pxdoc, pxf);
					RETURN_FALSE;
				}
				convert_to_long_ex(value);
				pxf[i].px_flen = Z_LVAL_PP(value);
				break;
			case 'D':
				pxf[i].px_ftype = pxfDate;
				pxf[i].px_flen = 4;
				break;
			case 'T':
				pxf[i].px_ftype = pxfTime;
				pxf[i].px_flen = 4;
				break;
			case '@':
				pxf[i].px_ftype = pxfTimestamp;
				pxf[i].px_flen = 8;
				break;
			case 'N':
				pxf[i].px_ftype = pxfNumber;
				pxf[i].px_flen = 8;
				break;
			case '$':
				pxf[i].px_ftype = pxfCurrency;
				pxf[i].px_flen = 8;
				break;
			case '#':
				pxf[i].px_ftype = pxfBCD;
				/* decimal point */
				if (zend_hash_index_find(lht2, 2, (void **)&value) == FAILURE) {
					php_error_docref(NULL TSRMLS_CC, E_ERROR, "expected field length as third element of list in field %d", i);
					px_efree(pxdoc, pxf[i].px_fname);
					px_efree(pxdoc, pxf);
					RETURN_FALSE;
				}
				convert_to_long_ex(value);
				pxf[i].px_flen = Z_LVAL_PP(value);
				break;
			case 'Y':
				pxf[i].px_ftype = pxfBytes;
				/* decimal point */
				if (zend_hash_index_find(lht2, 2, (void **)&value) == FAILURE) {
					php_error_docref(NULL TSRMLS_CC, E_ERROR, "expected field length as third element of list in field %d", i);
					px_efree(pxdoc, pxf[i].px_fname);
					px_efree(pxdoc, pxf);
					RETURN_FALSE;
				}
				convert_to_long_ex(value);
				pxf[i].px_flen = Z_LVAL_PP(value);
				break;
			case 'A':
				pxf[i].px_ftype = pxfAlpha;
				/* field length */
				if (zend_hash_index_find(lht2, 2, (void **)&value) == FAILURE) {
					php_error_docref(NULL TSRMLS_CC, E_ERROR, "expected field length as third element of list in field %d", i);
					px_efree(pxdoc, pxf[i].px_fname);
					px_efree(pxdoc, pxf);
					RETURN_FALSE;
				}
				convert_to_long_ex(value);
				pxf[i].px_flen = Z_LVAL_PP(value);
				break;
			default:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "unknown field type '%c'", str[0]);
			}
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expected array as field specification number %d.", i);
		}
		zend_hash_move_forward(lht);
	}

	if (PX_create_fp(pxdoc, pxf, numfields, fp, pxfFileTypNonIndexDB) < 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not create paradox file");
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool px_close(resource pxdoc)
   Closes the paradox document */
PHP_FUNCTION(px_close)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	zval *object = getThis();

	if (object) {
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zpx)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	PX_close(pxdoc);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool px_delete(resource pxdoc)
   Deletes the paradox document */
PHP_FUNCTION(px_delete)
{
	zval *zpx;
	pxdoc_t *pxdoc;

	/* This function cannot be called as a method of class Paradox because
	 * it is not listed as one. That's why we don't have to handle this case.
	 */
	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zpx)) {
		return;
	}

	PXDOC_FROM_ZVAL(pxdoc, &zpx);

	zend_list_delete(Z_RESVAL_P(zpx));

	RETURN_TRUE;
}
/* }}} */

#ifdef HAVE_PXPACK
/* {{{ proto bool px_pack(resource pxdoc)
   Reduce paradox document to smallest possible size. */
PHP_FUNCTION(px_pack)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	zval *object = getThis();

	if (object) {
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zpx)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	PX_pack(pxdoc);

	RETURN_TRUE;
}
/* }}} */
#endif

/* {{{ proto array px_get_record(resource pxdoc, int recno[, int type])
   Returns a record from the database */
PHP_FUNCTION(px_get_record)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	pxhead_t *pxh;
	pxfield_t *pxf;
	char *selectedfields;
	char *data;
	int i, offset;
	int recno = 0;
	int result_type = 0;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l", &recno, &result_type)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl|l", &zpx, &recno, &result_type)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	pxh = pxdoc->px_head;

	if((selectedfields = (char *) px_emalloc(pxdoc, pxh->px_numfields, "px_get_record: Allocate memory for array of selected fields.")) == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not allocate memory for array of selected fields.");
		RETURN_FALSE;
	}
	memset(selectedfields, '\0', pxh->px_numfields);
	pxf = pxh->px_fields;
	for(i=0; i<pxh->px_numfields; i++) {
		selectedfields[i] = 1;
		pxf++;
	}

	if((data = (char *) px_emalloc(pxdoc, pxh->px_recordsize, "px_get_record: Allocate memory for record.")) == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not allocate memory for record data.");
		if(selectedfields) 
			px_efree(pxdoc, selectedfields);
		RETURN_FALSE;
	}

	if(PX_get_record(pxdoc, recno, data)) {
		char *fname;
		array_init(return_value);
		pxf = pxh->px_fields;
		offset = 0;
		for(i=0; i<pxh->px_numfields; i++) {
			if(selectedfields[i]) {
				int len;
				switch(result_type) {
					case PX_KEYTOLOWER:
						fname =  php_strtolower(pxf->px_fname, strlen(pxf->px_fname));
						break;
					case PX_KEYTOUPPER:
						fname =  php_strtoupper(pxf->px_fname, strlen(pxf->px_fname));
						break;
					default:
						fname = pxf->px_fname;
				}
				switch(pxf->px_ftype) {
					case pxfAlpha: {
						char *value;
						if(0 < PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value)) {
							len = strlen(value) < pxf->px_flen ? strlen(value) : pxf->px_flen;
							add_assoc_stringl(return_value, fname, value, len, 0);
						} else {
							add_assoc_null(return_value, fname);
						}
						break;
					}
					case pxfShort: {
						short int value;
						if(0 < PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value)) {
							add_assoc_long(return_value, fname, value);
						} else {
							add_assoc_null(return_value, fname);
						}
						break;
					}
					case pxfAutoInc:
					case pxfTime:
					case pxfDate:
					case pxfLong: {
						long value;
						if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
							add_assoc_long(return_value, fname, value);
						} else {
							add_assoc_null(return_value, fname);
						}
						break;
					}
					case pxfCurrency:
					case pxfTimestamp:
					case pxfNumber: {
						double value;
						if(0 < PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value)) {
							add_assoc_double(return_value, fname, value);
						} else {
							add_assoc_null(return_value, fname);
						}
						break;
					}
					case pxfLogical: {
						char value;
						if(0 < PX_get_data_byte(pxdoc, &data[offset], pxf->px_flen, &value)) {
							if(value)
								add_assoc_bool(return_value, fname, 1);
							else
								add_assoc_bool(return_value, fname, 0);
						} else {
							add_assoc_null(return_value, fname);
						}

						break;
					}
					case pxfBCD: {
						char *value;
						if(0 < PX_get_data_bcd(pxdoc, (unsigned char *) &data[offset], pxf->px_fdc, &value)) {
							len = strlen(value);
							add_assoc_stringl(return_value, fname, value, len, 0);
						} else {
							add_assoc_null(return_value, fname);
						}
						break;
					}
					case pxfGraphic:
					case pxfBLOb:
					case pxfFmtMemoBLOb:
					case pxfMemoBLOb:
					case pxfOLE: {
						char *blobdata;
						int mod_nr, size;
						int ret;
						if(pxf->px_ftype == pxfGraphic)
							ret = PX_get_data_graphic(pxdoc, &data[offset], pxf->px_flen, &mod_nr, &size, &blobdata);
						else
							ret = PX_get_data_blob(pxdoc, &data[offset], pxf->px_flen, &mod_nr, &size, &blobdata);
						if(0 < ret) {
							add_assoc_stringl(return_value, fname, blobdata, size, 1);
							px_efree(pxdoc, blobdata);
						} else {
							add_assoc_null(return_value, fname);
						}
						break;
					}
				}
				offset += pxf->px_flen;
				pxf++;
			}
		}
	}
	px_efree(pxdoc, selectedfields);
	px_efree(pxdoc, data);
}
/* }}} */

/* {{{ create_record(pxdoc_t *pxdoc, HashTable *lht)
 */
char *create_record(pxdoc_t *pxdoc, HashTable *lht TSRMLS_DC) {
	pxhead_t *pxh;
	pxfield_t *pxf;
	int i, offset, numfields;
	char *data;

	pxh = pxdoc->px_head;
	pxf = pxh->px_fields;

	if(0 == (numfields = zend_hash_num_elements(lht))) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "the data array is empty");
		return NULL;
	}
	/* Don't take more columns into account then columns in the database */
	if(numfields > pxh->px_numfields)
		numfields = pxh->px_numfields;

	if(NULL == (data = (char *) px_emalloc(pxdoc, pxh->px_recordsize, "px_put_record: Allocate memory for record data."))) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not allocate memory for record data");
		return NULL;
	}
	memset(data, 0, pxh->px_recordsize);

	offset = 0;
	for(i=0; i<numfields; i++) {
		zval **keydataptr;
		zend_hash_get_current_data(lht, (void **) &keydataptr);
		if(Z_TYPE_PP(keydataptr) != IS_NULL) {
			switch(pxf[i].px_ftype) {
				case pxfAlpha:
					convert_to_string_ex(keydataptr);
					PX_put_data_alpha(pxdoc, &data[offset], pxf[i].px_flen, Z_STRVAL_PP(keydataptr));
					break;
				case pxfShort:
					convert_to_long_ex(keydataptr);
					PX_put_data_short(pxdoc, &data[offset], pxf[i].px_flen, Z_LVAL_PP(keydataptr));
					break;
				case pxfDate:
				case pxfTime:
				case pxfAutoInc:
				case pxfLong:
					convert_to_long_ex(keydataptr);
					PX_put_data_long(pxdoc, &data[offset], pxf[i].px_flen, Z_LVAL_PP(keydataptr));
					break;
				case pxfLogical: {
					char c;
					convert_to_long_ex(keydataptr);
					c = (char) Z_LVAL_PP(keydataptr);
					PX_put_data_byte(pxdoc, &data[offset], 1, c);
					break;
				}
				case pxfCurrency:
				case pxfTimestamp:
				case pxfNumber:
					convert_to_double_ex(keydataptr);
					PX_put_data_double(pxdoc, &data[offset], pxf[i].px_flen, Z_DVAL_PP(keydataptr));
					break;

			}
		}
		offset += pxf[i].px_flen;
		zend_hash_move_forward(lht);
	}
	return data;
}
/* }}} */

/* {{{ create_record2(pxdoc_t *pxdoc, HashTable *lht)
 */
pxval_t **create_record2(pxdoc_t *pxdoc, HashTable *lht TSRMLS_DC) {
	pxhead_t *pxh;
	pxfield_t *pxf;
	int i, numfields;
	pxval_t **dataptr;

	pxh = pxdoc->px_head;
	pxf = pxh->px_fields;

	if(0 == (numfields = zend_hash_num_elements(lht))) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "the data array is empty");
		return NULL;
	}
	if(numfields > pxh->px_numfields) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Record array has more elements than fields in database");
		return NULL;
	}

	if(NULL == (dataptr = (pxval_t **) px_emalloc(pxdoc, pxh->px_numfields*sizeof(pxval_t *), "px_put_record: Allocate memory for record data."))) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not allocate memory for record data");
		return NULL;
	}
	memset(dataptr, 0, pxh->px_numfields*sizeof(pxval_t *));

	for(i=0; i<pxh->px_numfields; i++) {
		zval **keydataptr;
		MAKE_PXVAL(pxdoc, dataptr[i]);
		if(zend_hash_num_elements(lht) <= i) {
			dataptr[i]->isnull = 1;
		} else {
			zend_hash_get_current_data(lht, (void **) &keydataptr);
			if(i <= numfields && Z_TYPE_PP(keydataptr) != IS_NULL) {
				switch(pxf[i].px_ftype) {
					case pxfAlpha:
					case pxfMemoBLOb:
						convert_to_string_ex(keydataptr);
						dataptr[i]->value.str.len = Z_STRLEN_PP(keydataptr);
						dataptr[i]->value.str.val = Z_STRVAL_PP(keydataptr);
						break;
					case pxfShort:
						convert_to_long_ex(keydataptr);
						dataptr[i]->value.lval = Z_LVAL_PP(keydataptr);
						break;
					case pxfDate:
					case pxfTime:
					case pxfAutoInc:
					case pxfLong:
						convert_to_long_ex(keydataptr);
						dataptr[i]->value.lval = Z_LVAL_PP(keydataptr);
						break;
					case pxfLogical: {
						convert_to_long_ex(keydataptr);
						dataptr[i]->value.lval = Z_LVAL_PP(keydataptr);
						break;
					}
					case pxfCurrency:
					case pxfTimestamp:
					case pxfNumber:
						convert_to_double_ex(keydataptr);
						dataptr[i]->value.dval = Z_DVAL_PP(keydataptr);
						break;
					default:
						dataptr[i]->isnull = 1;
				}
			} else {
				dataptr[i]->isnull = 1;
			}
			zend_hash_move_forward(lht);
		}
	}
	return dataptr;
}
/* }}} */

/* {{{ proto bool px_put_record(resource pxdoc, array data [, int position])
   Stores a record into the database */
PHP_FUNCTION(px_put_record)
{
	zval *zpx, *record;
	HashTable *lht;
	pxdoc_t *pxdoc;
	char *data;
	int recpos=-1;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|l", &record, &recpos)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz|l", &zpx, &record, &recpos)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if (Z_TYPE_P(record) != IS_ARRAY) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expected array as second parameter");
		RETURN_FALSE;
	}

	lht = Z_ARRVAL_P(record);

	if(NULL == (data = create_record(pxdoc, lht TSRMLS_CC))) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Creating database record failed");
		RETURN_FALSE;
	}

	if(recpos < 0)
		PX_put_record(pxdoc, data);
	else
		PX_put_recordn(pxdoc, data, recpos);
	efree(data);

	RETURN_TRUE;
}
/* }}} */

#ifdef HAVE_PXRETRIEVERECORD
/* {{{ proto array px_retrieve_record(resource pxdoc, int recno[, int type])
   Returns a record from the database */
PHP_FUNCTION(px_retrieve_record)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	pxhead_t *pxh;
	pxval_t **dataptr;
	int i;
	int recno = 0;
	int result_type = 0;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l", &recno, &result_type)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl|l", &zpx, &recno, &result_type)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	pxh = pxdoc->px_head;

	if(NULL != (dataptr = PX_retrieve_record(pxdoc, recno))) {
		char *fname;
		pxfield_t *pxf;

		pxf = pxh->px_fields;
		array_init(return_value);
		for(i=0; i<PX_get_num_fields(pxdoc); i++) {
			switch(result_type) {
				case PX_KEYTOLOWER:
					fname =  php_strtolower(pxf->px_fname, strlen(pxf->px_fname));
					break;
				case PX_KEYTOUPPER:
					fname =  php_strtoupper(pxf->px_fname, strlen(pxf->px_fname));
					break;
				default:
					fname = pxf->px_fname;
			}
			switch(dataptr[i]->type) {
				case pxfAlpha:
				case pxfBCD:
				case pxfGraphic:
				case pxfBLOb:
				case pxfFmtMemoBLOb:
				case pxfMemoBLOb:
				case pxfOLE: {
					if((dataptr[i]->isnull == 0) && dataptr[i]->value.str.val) {
						/* Make a copy of the string because it adds a terminating \0
						 * char at the end. dataptr[i]->value.str.val doesn't have one
						 */
						add_assoc_stringl(return_value, fname, dataptr[i]->value.str.val, dataptr[i]->value.str.len, 1);
						efree(dataptr[i]->value.str.val);
					} else {
						add_assoc_null(return_value, fname);
					}
					break;
				}
				case pxfDate:
				case pxfAutoInc:
				case pxfTime:
				case pxfShort:
				case pxfLong: {
					if(dataptr[i]->isnull == 0) {
						add_assoc_long(return_value, fname, dataptr[i]->value.lval);
					} else {
						add_assoc_null(return_value, fname);
					}
					break;
				}
				case pxfCurrency:
				case pxfTimestamp:
				case pxfNumber: {
					if(dataptr[i]->isnull == 0) {
						add_assoc_double(return_value, fname, dataptr[i]->value.dval);
					} else {
						add_assoc_null(return_value, fname);
					}
					break;
				}
				case pxfLogical: {
					if(dataptr[i]->isnull == 0) {
						add_assoc_bool(return_value, fname, dataptr[i]->value.lval);
					} else {
						add_assoc_null(return_value, fname);
					}

					break;
				}
			}
			pxf++;
		}
		for(i=0; i<PX_get_num_fields(pxdoc); i++) {
			FREE_PXVAL(pxdoc, dataptr[i]);
		}
		pxdoc->free(pxdoc, dataptr);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Could not retrieve record number %d.", recno);
		RETURN_FALSE;
	}
}
/* }}} */
#endif

#ifdef HAVE_PXINSERTRECORD
/* {{{ proto int px_insert_record(resource pxdoc, array data)
   Stores a record into the database */
PHP_FUNCTION(px_insert_record)
{
	zval *zpx, *record;
	HashTable *lht;
	pxdoc_t *pxdoc;
	pxval_t **dataptr;
	int ret, i;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &record)) {
			WRONG_PARAM_COUNT;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz", &zpx, &record)) {
			WRONG_PARAM_COUNT;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if (Z_TYPE_P(record) != IS_ARRAY) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expected array as second parameter");
		RETURN_FALSE;
	}

	lht = Z_ARRVAL_P(record);

	if(NULL == (dataptr = create_record2(pxdoc, lht TSRMLS_CC))) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Creating database record failed");
		RETURN_FALSE;
	}

	ret = PX_insert_record(pxdoc, dataptr);
	for(i=0; i<PX_get_num_fields(pxdoc); i++) {
		FREE_PXVAL(pxdoc, dataptr[i]);
	}
	efree(dataptr);
	if(0 > ret) {
		RETURN_FALSE;
	} else {
		RETURN_LONG(ret);
	}

}
/* }}} */
#endif

#ifdef HAVE_PXDELETERECORD
/* {{{ proto bool px_delete_record(resource pxdoc, int position)
   Removes a record from the database */
PHP_FUNCTION(px_delete_record)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	pxhead_t *pxh;
	pxfield_t *pxf;
	int recpos=-1;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &recpos)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zpx, &recpos)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	pxh = pxdoc->px_head;
	pxf = pxh->px_fields;

	if(0 > PX_delete_record(pxdoc, recpos)) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */
#endif

#ifdef HAVE_PXUPDATERECORD
/* {{{ proto bool px_update_record(resource pxdoc, array data, int recno)
   Updates a record into the database */
PHP_FUNCTION(px_update_record)
{
	zval *zpx, *record;
	HashTable *lht;
	pxdoc_t *pxdoc;
	pxval_t **dataptr;
	int recpos=-1;
	int ret, i;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zl", &record, &recpos)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rzl", &zpx, &record, &recpos)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if (Z_TYPE_P(record) != IS_ARRAY) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expected array as second parameter");
		RETURN_FALSE;
	}

	lht = Z_ARRVAL_P(record);

	if(NULL == (dataptr = create_record2(pxdoc, lht TSRMLS_CC))) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Creating database record failed");
		RETURN_FALSE;
	}

	ret = PX_update_record(pxdoc, dataptr, recpos);
	for(i=0; i<PX_get_num_fields(pxdoc); i++) {
		FREE_PXVAL(pxdoc, dataptr[i]);
	}
	efree(dataptr);
	if(0 > ret) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}

	RETURN_TRUE;
}
/* }}} */
#endif

/* {{{ proto int px_numrecords(resource pxdoc)
   Returns the number of records (columns) in the database */
PHP_FUNCTION(px_numrecords)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	int num;
	zval *object = getThis();

	if (object) {
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zpx)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if((num = PX_get_num_records(pxdoc)) < 0) {
		RETURN_FALSE;
	} else {
		RETURN_LONG(num);
	}
}
/* }}} */

/* {{{ proto int px_numfields(resource pxdoc)
   Returns the number of fields (columns) in the database */
PHP_FUNCTION(px_numfields)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	int num;
	zval *object = getThis();

	if (object) {
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zpx)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if((num = PX_get_num_fields(pxdoc)) < 0) {
		RETURN_FALSE;
	} else {
		RETURN_LONG(num);
	}
}
/* }}} */

/* {{{ proto array px_get_field(resource pxdoc, int fieldno)
   Returns meta data of a field */
PHP_FUNCTION(px_get_field)
{
	zval *zpx;
	long field;
	pxdoc_t *pxdoc;
	pxfield_t *pxf;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &field)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zpx, &field)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	pxf = PX_get_field(pxdoc, field);

	array_init(return_value);
	add_assoc_string(return_value, "name", pxf->px_fname, 1);
	add_assoc_long(return_value, "type", pxf->px_ftype);
	add_assoc_long(return_value, "size", pxf->px_flen);
}
/* }}} */

/* {{{ proto array px_get_schema(resource pxdoc[, int type])
   Returns meta data of a table */
PHP_FUNCTION(px_get_schema)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	pxfield_t *pxf, *pxf1;
	int i, fn;
	int result_type = 0;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &result_type)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|l", &zpx, &result_type)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	pxf = pxf1 = PX_get_fields(pxdoc);
	fn = PX_get_num_fields(pxdoc);
	if((pxf1 == NULL) || (fn < 0)) {
		RETURN_FALSE;
	}

	array_init(return_value);
	for(i=0; i<fn; i++) {
		char *fname;
		zval *fieldarr;
		MAKE_STD_ZVAL(fieldarr);
		array_init(fieldarr);
		add_assoc_long(fieldarr, "type", pxf->px_ftype);
		add_assoc_long(fieldarr, "size", pxf->px_flen);
		switch(result_type) {
			case PX_KEYTOLOWER:
				fname =  php_strtolower(pxf->px_fname, strlen(pxf->px_fname));
				break;
			case PX_KEYTOUPPER:
				fname =  php_strtoupper(pxf->px_fname, strlen(pxf->px_fname));
				break;
			default:
				fname = pxf->px_fname;
		}
		zend_hash_add(Z_ARRVAL_P(return_value), fname, strlen(fname)+1, &fieldarr, sizeof(zval *), NULL);
		pxf++;
	}
}
/* }}} */

/* {{{ proto array px_get_info(resource pxdoc)
   Returns associated array with information from the database header */
PHP_FUNCTION(px_get_info)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	pxhead_t *pxh;
	zval *object = getThis();

	if (object) {
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zpx)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	pxh = pxdoc->px_head;
	array_init(return_value);
	add_assoc_long(return_value, "fileversion", pxh->px_fileversion);
	if(pxh->px_tablename)
		add_assoc_string(return_value, "tablename", pxh->px_tablename, strlen(pxh->px_tablename));
	add_assoc_long(return_value, "numrecords", pxh->px_numrecords);
	add_assoc_long(return_value, "numfields", pxh->px_numfields);
	add_assoc_long(return_value, "headersize", pxh->px_headersize);
	add_assoc_long(return_value, "recordsize", pxh->px_recordsize);
	add_assoc_long(return_value, "maxtablesize", pxh->px_maxtablesize);
	add_assoc_long(return_value, "numdatablocks", pxh->px_fileblocks);
	add_assoc_long(return_value, "numindexfields", pxh->px_indexfieldnumber);
	add_assoc_long(return_value, "codepage", pxh->px_doscodepage);
}
/* }}} */

/* {{{ proto bool px_set_parameter(resource pxdoc, string name, string value)
   Sets certain parameters. */
PHP_FUNCTION(px_set_parameter)
{
	zval *zpx;
	char *name, *value;
	long name_len, value_len;
	pxdoc_t *pxdoc;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &name, &name_len, &value, &value_len)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zpx, &name, &name_len, &value, &value_len)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if(0 > PX_set_parameter(pxdoc, name, value)) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */

/* {{{ proto bool px_get_parameter(resource pxdoc, string name)
   Returns the value of a parameter. */
PHP_FUNCTION(px_get_parameter)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	char *name;
	long name_len;
	char *value;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zpx, &name, &name_len)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if(0 > PX_get_parameter(pxdoc, name, &value)) {
		RETURN_FALSE;
	} else {
		RETURN_STRING(value, 1);
	}
}
/* }}} */

/* {{{ proto bool px_set_value(resource pxdoc, string name, float value)
   Sets certain values. */
PHP_FUNCTION(px_set_value)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	char *name;
	long name_len;
	double value;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sd", &name, &name_len, &value)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsd", &zpx, &name, &name_len, &value)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if(0 > PX_set_value(pxdoc, name, value)) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */

/* {{{ proto bool px_get_value(resource pxdoc, string name)
   Returns the value of a value. */
PHP_FUNCTION(px_get_value)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	char *name;
	long name_len;
	float value;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zpx, &name, &name_len)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if(0 > PX_get_value(pxdoc, name, &value)) {
		RETURN_FALSE;
	} else {
		RETURN_DOUBLE(value);
	}
}
/* }}} */

/* {{{ proto bool px_set_targetencoding(resource pxdoc, string encoding)
   Sets encoding of field data when it is retrieved. */
PHP_FUNCTION(px_set_targetencoding)
{
	zval *zpx;
	pxdoc_t *pxdoc;
	char *encoding;
	long encoding_len;
	int ret;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &encoding, &encoding_len)) {
			RETURN_FALSE;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zpx, &encoding, &encoding_len)) {
			RETURN_FALSE;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	ret = PX_set_targetencoding(pxdoc, encoding);
	if(ret == -2) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "pxlib does not support recoding.");
		RETURN_FALSE;
	} else if(ret < 0) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */

/* {{{ proto void px_set_tablename(resource pxdoc, string tablename)
   Sets name of table as stored in database file. Calling this function
   makes only sense if you create a database and if you call it before
   PX_create_fp(). */
PHP_FUNCTION(px_set_tablename)
{
	zval *zpx;
	char *name;
	long name_len;
	pxdoc_t *pxdoc;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len)) {
			return;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zpx, &name, &name_len)) {
			return;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	PX_set_tablename(pxdoc, name);
}
/* }}} */

/* {{{ proto bool px_set_blob_file(resource pxdoc, string filename)
   Loads file as a data source for blobs.  */
PHP_FUNCTION(px_set_blob_file)
{
	zval *zpx;
	char *name;
	long name_len;
	pxdoc_t *pxdoc;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len)) {
			RETURN_FALSE;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zpx, &name, &name_len)) {
			RETURN_FALSE;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if(0 > PX_set_blob_file(pxdoc, name)) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */

/* {{{ proto bool px_timestamp2string(resource pxdoc, float value, string format)
   Converts the timestamp into a string.  */
PHP_FUNCTION(px_timestamp2string)
{
	zval *zpx;
	char *format, *datestr;
	long format_len;
	double value;
	pxdoc_t *pxdoc;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ds", &value, &format, &format_len)) {
			RETURN_FALSE;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rds", &zpx, &value, &format, &format_len)) {
			RETURN_FALSE;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if(value == 0) {
		RETURN_EMPTY_STRING();
	}
	if(NULL == (datestr = PX_timestamp2string(pxdoc, value, format))) {
		RETURN_FALSE;
	} else {
		RETURN_STRING(datestr, 0);
	}
}
/* }}} */

/* {{{ proto bool px_date2string(resource pxdoc, long value, string format)
   Converts the date into a string.  */
PHP_FUNCTION(px_date2string)
{
	zval *zpx;
	char *format, *datestr;
	long format_len;
	long value;
	pxdoc_t *pxdoc;
	zval *object = getThis();

	if (object) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &value, &format, &format_len)) {
			RETURN_FALSE;
		}
		PXDOC_FROM_OBJECT(pxdoc, object);
	} else {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rls", &zpx, &value, &format, &format_len)) {
			RETURN_FALSE;
		}
		PXDOC_FROM_ZVAL(pxdoc, &zpx);
	}

	if(value == 0) {
		RETURN_EMPTY_STRING();
	}
	if(NULL == (datestr = PX_date2string(pxdoc, value, format))) {
		RETURN_FALSE;
	} else {
		RETURN_STRING(datestr, 0);
	}
}
/* }}} */

#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
