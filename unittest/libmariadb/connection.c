/*
Copyright (c) 2009, 2010, Oracle and/or its affiliates. All rights reserved.

The MySQL Connector/C is licensed under the terms of the GPLv2
<http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
MySQL Connectors. There are special exceptions to the terms and
conditions of the GPLv2 as it is applied to this software, see the
FLOSS License Exception
<http://www.mysql.com/about/legal/licensing/foss-exception.html>.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/
/**
  Some basic tests of the client API.
*/

#include "my_test.h"

static int test_conc66(MYSQL *my)
{
  MYSQL *mysql= mysql_init(NULL);
  int rc;
  FILE *fp;

  if (!(fp= fopen("./my.cnf", "w")))
    return FAIL;

  fprintf(fp, "[conc-66]\n");
  fprintf(fp, "user=conc66\n");
  fprintf(fp, "password='test\\\";#test'\n");

  fclose(fp);

  rc= mysql_options(mysql, MYSQL_READ_DEFAULT_GROUP, "conc-66");
  check_mysql_rc(rc, mysql);
  rc= mysql_options(mysql, MYSQL_READ_DEFAULT_FILE, "./my.cnf");
  check_mysql_rc(rc, mysql);

  rc= mysql_query(my, "GRANT ALL ON test.* TO 'conc66'@'localhost' IDENTIFIED BY 'test\";#test'");
  check_mysql_rc(rc, my);
  rc= mysql_query(my, "FLUSH PRIVILEGES");
  check_mysql_rc(rc, my);
  if (!mysql_real_connect(mysql, hostname, NULL,
                             NULL, schema, port, socketname, 0))
  {
    diag("Error: %s", mysql_error(mysql));
    return FAIL;
  }
  rc= mysql_query(my, "DROP USER conc66@localhost");

  check_mysql_rc(rc, my);
  mysql_close(mysql);
  return OK; 
}

static int test_bug20023(MYSQL *mysql)
{
  int sql_big_selects_orig;
  int max_join_size_orig;

  int sql_big_selects_2;
  int sql_big_selects_3;
  int sql_big_selects_4;
  int sql_big_selects_5;
  int rc;

  if (mysql_get_server_version(mysql) < 50100) {
    diag("Test requires MySQL Server version 5.1 or above");
    return SKIP;
  }

  /***********************************************************************
    Remember original SQL_BIG_SELECTS, MAX_JOIN_SIZE values.
  ***********************************************************************/

  query_int_variable(mysql,
                     "@@session.sql_big_selects",
                     &sql_big_selects_orig);

  query_int_variable(mysql,
                     "@@global.max_join_size",
                     &max_join_size_orig);

  /***********************************************************************
    Test that COM_CHANGE_USER resets the SQL_BIG_SELECTS to the initial value.
  ***********************************************************************/

  /* Issue COM_CHANGE_USER. */
  rc= mysql_change_user(mysql, username, password, schema);
  check_mysql_rc(rc, mysql);

  /* Query SQL_BIG_SELECTS. */

  query_int_variable(mysql,
                     "@@session.sql_big_selects",
                     &sql_big_selects_2);

  /* Check that SQL_BIG_SELECTS is reset properly. */

  FAIL_UNLESS(sql_big_selects_orig == sql_big_selects_2, "Different value for sql_big_select");

  /***********************************************************************
    Test that if MAX_JOIN_SIZE set to non-default value,
    SQL_BIG_SELECTS will be 0.
  ***********************************************************************/

  /* Set MAX_JOIN_SIZE to some non-default value. */

  rc= mysql_query(mysql, "SET @@global.max_join_size = 10000");
  check_mysql_rc(rc, mysql);
  rc= mysql_query(mysql, "SET @@session.max_join_size = default");
  check_mysql_rc(rc, mysql);

  /* Issue COM_CHANGE_USER. */

  rc= mysql_change_user(mysql, username, password, schema);
  check_mysql_rc(rc, mysql);

  /* Query SQL_BIG_SELECTS. */

  query_int_variable(mysql,
                     "@@session.sql_big_selects",
                     &sql_big_selects_3);

  /* Check that SQL_BIG_SELECTS is 0. */

  FAIL_UNLESS(sql_big_selects_3 == 0, "big_selects != 0");

  /***********************************************************************
    Test that if MAX_JOIN_SIZE set to default value,
    SQL_BIG_SELECTS will be 1.
  ***********************************************************************/

  /* Set MAX_JOIN_SIZE to the default value (-1). */

  rc= mysql_query(mysql, "SET @@global.max_join_size = cast(-1 as unsigned int)");
  rc= mysql_query(mysql, "SET @@session.max_join_size = default");

  /* Issue COM_CHANGE_USER. */

  rc= mysql_change_user(mysql, username, password, schema);
  check_mysql_rc(rc, mysql);

  /* Query SQL_BIG_SELECTS. */

  query_int_variable(mysql,
                     "@@session.sql_big_selects",
                     &sql_big_selects_4);

  /* Check that SQL_BIG_SELECTS is 1. */

  FAIL_UNLESS(sql_big_selects_4 == 1, "sql_big_select != 1");

  /***********************************************************************
    Restore MAX_JOIN_SIZE.
    Check that SQL_BIG_SELECTS will be the original one.
  ***********************************************************************/

  rc= mysql_query(mysql, "SET @@global.max_join_size = cast(-1 as unsigned int)");
  check_mysql_rc(rc, mysql);

  rc= mysql_query(mysql, "SET @@session.max_join_size = default");
  check_mysql_rc(rc, mysql);

  /* Issue COM_CHANGE_USER. */

  rc= mysql_change_user(mysql, username, password, schema);
  check_mysql_rc(rc, mysql);

  /* Query SQL_BIG_SELECTS. */

  query_int_variable(mysql,
                     "@@session.sql_big_selects",
                     &sql_big_selects_5);

  /* Check that SQL_BIG_SELECTS is 1. */

  FAIL_UNLESS(sql_big_selects_5 == sql_big_selects_orig, "big_select != 1");

  /***********************************************************************
    That's it. Cleanup.
  ***********************************************************************/

  return OK;
}

static int test_change_user(MYSQL *mysql)
{
  char buff[256];
  const char *user_pw= "mysqltest_pw";
  const char *user_no_pw= "mysqltest_no_pw";
  const char *pw= "password";
  const char *db= "mysqltest_user_test_database";
  int rc;

  diag("Due to mysql_change_user security fix this test will not work anymore.");
  return(SKIP);

  /* Prepare environment */
  sprintf(buff, "drop database if exists %s", db);
  rc= mysql_query(mysql, buff);
  check_mysql_rc(rc, mysql)

  sprintf(buff, "create database %s", db);
  rc= mysql_query(mysql, buff);
  check_mysql_rc(rc, mysql)

  sprintf(buff,
          "grant select on %s.* to %s@'%%' identified by '%s'",
          db,
          user_pw,
          pw);
  rc= mysql_query(mysql, buff);
  check_mysql_rc(rc, mysql)

  sprintf(buff,
          "grant select on %s.* to %s@'%%'",
          db,
          user_no_pw);
  rc= mysql_query(mysql, buff);
  check_mysql_rc(rc, mysql)


  /* Try some combinations */
  rc= mysql_change_user(mysql, NULL, NULL, NULL);
  FAIL_UNLESS(rc, "Error expected");


  rc= mysql_change_user(mysql, "", NULL, NULL);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, "", "", NULL);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, "", "", "");
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, NULL, "", "");
  FAIL_UNLESS(rc, "Error expected");


  rc= mysql_change_user(mysql, NULL, NULL, "");
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, "", NULL, "");
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, user_pw, NULL, "");
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, user_pw, "", "");
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, user_pw, "", NULL);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, user_pw, NULL, NULL);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, user_pw, "", db);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, user_pw, NULL, db);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, user_pw, pw, db);
  check_mysql_rc(rc, mysql)

  rc= mysql_change_user(mysql, user_pw, pw, NULL);
  check_mysql_rc(rc, mysql)

  rc= mysql_change_user(mysql, user_pw, pw, "");
  check_mysql_rc(rc, mysql)

  rc= mysql_change_user(mysql, user_no_pw, pw, db);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, user_no_pw, pw, "");
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, user_no_pw, pw, NULL);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, user_no_pw, "", NULL);
  check_mysql_rc(rc, mysql)

  rc= mysql_change_user(mysql, user_no_pw, "", "");
  check_mysql_rc(rc, mysql)

  rc= mysql_change_user(mysql, user_no_pw, "", db);
  check_mysql_rc(rc, mysql)

  rc= mysql_change_user(mysql, user_no_pw, NULL, db);
  check_mysql_rc(rc, mysql)

  rc= mysql_change_user(mysql, "", pw, db);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, "", pw, "");
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, "", pw, NULL);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, NULL, pw, NULL);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, NULL, NULL, db);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, NULL, "", db);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, "", "", db);
  FAIL_UNLESS(rc, "Error expected");

  /* Cleanup the environment */

  rc= mysql_change_user(mysql, username, password, schema);
  check_mysql_rc(rc, mysql);

  sprintf(buff, "drop database %s", db);
  rc= mysql_query(mysql, buff);
  check_mysql_rc(rc, mysql)

  sprintf(buff, "drop user %s@'%%'", user_pw);
  rc= mysql_query(mysql, buff);
  check_mysql_rc(rc, mysql)

  sprintf(buff, "drop user %s@'%%'", user_no_pw);
  rc= mysql_query(mysql, buff);
  check_mysql_rc(rc, mysql)

  return OK;
}

/**
  Bug#31669 Buffer overflow in mysql_change_user()
*/

#define LARGE_BUFFER_SIZE 2048

static int test_bug31669(MYSQL *mysql)
{
  int rc;
  static char buff[LARGE_BUFFER_SIZE+1];
  static char user[USERNAME_CHAR_LENGTH+1];
  static char db[NAME_CHAR_LEN+1];
  static char query[LARGE_BUFFER_SIZE*2];

  diag("Due to mysql_change_user security fix this test will not work anymore.");
  return(SKIP);

  rc= mysql_change_user(mysql, NULL, NULL, NULL);
  FAIL_UNLESS(rc, "Error expected");

  rc= mysql_change_user(mysql, "", "", "");
  FAIL_UNLESS(rc, "Error expected");

  memset(buff, 'a', sizeof(buff));

  rc= mysql_change_user(mysql, buff, buff, buff);
  FAIL_UNLESS(rc, "Error epected");

  rc = mysql_change_user(mysql, username, password, schema);
  check_mysql_rc(rc, mysql);

  memset(db, 'a', sizeof(db));
  db[NAME_CHAR_LEN]= 0;
  sprintf(query, "CREATE DATABASE IF NOT EXISTS %s", db);
  rc= mysql_query(mysql, query);
  check_mysql_rc(rc, mysql);

  memset(user, 'b', sizeof(user));
  user[USERNAME_CHAR_LENGTH]= 0;
  memset(buff, 'c', sizeof(buff));
  buff[LARGE_BUFFER_SIZE]= 0;
  sprintf(query, "GRANT ALL PRIVILEGES ON *.* TO '%s'@'%%' IDENTIFIED BY '%s' WITH GRANT OPTION", user, buff);
  rc= mysql_query(mysql, query);
  check_mysql_rc(rc, mysql);

  rc= mysql_query(mysql, "FLUSH PRIVILEGES");
  check_mysql_rc(rc, mysql);

  rc= mysql_change_user(mysql, user, buff, db);
  check_mysql_rc(rc, mysql);

  user[USERNAME_CHAR_LENGTH-1]= 'a';
  rc= mysql_change_user(mysql, user, buff, db);
  FAIL_UNLESS(rc, "Error expected");

  user[USERNAME_CHAR_LENGTH-1]= 'b';
  buff[LARGE_BUFFER_SIZE-1]= 'd';
  rc= mysql_change_user(mysql, user, buff, db);
  FAIL_UNLESS(rc, "Error expected");

  buff[LARGE_BUFFER_SIZE-1]= 'c';
  db[NAME_CHAR_LEN-1]= 'e';
  rc= mysql_change_user(mysql, user, buff, db);
  FAIL_UNLESS(rc, "Error expected");

  db[NAME_CHAR_LEN-1]= 'a';
  rc= mysql_change_user(mysql, user, buff, db);
  FAIL_UNLESS(!rc, "Error expected");

  rc= mysql_change_user(mysql, user + 1, buff + 1, db + 1);
  FAIL_UNLESS(rc, "Error expected");

  rc = mysql_change_user(mysql, username, password, schema);
  check_mysql_rc(rc, mysql);

  sprintf(query, "DROP DATABASE %s", db);
  rc= mysql_query(mysql, query);
  check_mysql_rc(rc, mysql);

  sprintf(query, "DELETE FROM mysql.user WHERE User='%s'", user);
  rc= mysql_query(mysql, query);
  check_mysql_rc(rc, mysql);
  FAIL_UNLESS(mysql_affected_rows(mysql) == 1, "");

  return OK;
}

/**
     Bug# 33831 mysql_real_connect() should fail if
     given an already connected MYSQL handle.
*/

static int test_bug33831(MYSQL *mysql)
{
  FAIL_IF(mysql_real_connect(mysql, hostname, username,
                             password, schema, port, socketname, 0), 
         "Error expected");
  
  return OK;
}

/* Test MYSQL_OPT_RECONNECT, Bug#15719 */

static int test_opt_reconnect(MYSQL *mysql)
{
  my_bool my_true= TRUE;
  int rc;

  mysql= mysql_init(NULL);
  FAIL_IF(!mysql, "not enough memory");

  FAIL_UNLESS(mysql->reconnect == 0, "reconnect != 0");

  rc= mysql_options(mysql, MYSQL_OPT_RECONNECT, &my_true);
  check_mysql_rc(rc, mysql);

  FAIL_UNLESS(mysql->reconnect == 1, "reconnect != 1");

  if (!(mysql_real_connect(mysql, hostname, username,
                           password, schema, port,
                           socketname, 0)))
  {
    diag("connection failed");
    mysql_close(mysql);
    return FAIL;
  }

  FAIL_UNLESS(mysql->reconnect == 1, "reconnect != 1");

  mysql_close(mysql);

  mysql= mysql_init(NULL);
  FAIL_IF(!mysql, "not enough memory");

  FAIL_UNLESS(mysql->reconnect == 0, "reconnect != 0");

  if (!(mysql_real_connect(mysql, hostname, username,
                           password, schema, port,
                           socketname, 0)))
  {
    diag("connection failed");
    mysql_close(mysql);
    return FAIL;
  }

  FAIL_UNLESS(mysql->reconnect == 0, "reconnect != 0");

  mysql_close(mysql);
  return OK;
}


static int test_compress(MYSQL *mysql)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  int rc;

  mysql= mysql_init(NULL);
  FAIL_IF(!mysql, "not enough memory");

  /* use compressed protocol */
  rc= mysql_options(mysql, MYSQL_OPT_COMPRESS, NULL);



  if (!(mysql_real_connect(mysql, hostname, username,
                           password, schema, port,
                           socketname, 0)))
  {
    diag("connection failed");
    return FAIL;
  }

  rc= mysql_query(mysql, "SHOW STATUS LIKE 'compression'");
  check_mysql_rc(rc, mysql);
  res= mysql_store_result(mysql);
  row= mysql_fetch_row(res);
  FAIL_UNLESS(strcmp(row[1], "ON") == 0, "Compression off");
  mysql_free_result(res);

  mysql_close(mysql);
  return OK;
}

static int test_reconnect(MYSQL *mysql)
{
  my_bool my_true= TRUE;
  MYSQL *mysql1;
  int rc;

  mysql1= mysql_init(NULL);
  FAIL_IF(!mysql1, "not enough memory");

  FAIL_UNLESS(mysql1->reconnect == 0, "reconnect != 0");

  rc= mysql_options(mysql1, MYSQL_OPT_RECONNECT, &my_true);
  check_mysql_rc(rc, mysql1);

  FAIL_UNLESS(mysql1->reconnect == 1, "reconnect != 1");

  if (!(mysql_real_connect(mysql1, hostname, username,
                           password, schema, port,
                           socketname, 0)))
  {
    diag("connection failed");
    mysql_close(mysql);
    return FAIL;
  }

  FAIL_UNLESS(mysql1->reconnect == 1, "reconnect != 1");

  diag("Thread_id before kill: %lu", mysql_thread_id(mysql1));
  mysql_kill(mysql, mysql_thread_id(mysql1));
  sleep(4);

  rc= mysql_query(mysql1, "SELECT 1 FROM DUAL LIMIT 0");
  check_mysql_rc(rc, mysql1);
  diag("Thread_id after kill: %lu", mysql_thread_id(mysql1));

  FAIL_UNLESS(mysql1->reconnect == 1, "reconnect != 1");
  mysql_close(mysql1);
  return OK;
}

int test_conc21(MYSQL *mysql)
{
  int rc;
  MYSQL_RES *res= NULL;
  MYSQL_ROW row;
  char tmp[256];
  int check_server_version= 0;
  int major=0, minor= 0, patch=0;

  rc= mysql_query(mysql, "SELECT @@version");
  check_mysql_rc(rc, mysql);

  res= mysql_store_result(mysql);
  FAIL_IF(res == NULL, "invalid result set");

  row= mysql_fetch_row(res);
  strcpy(tmp, row[0]);
  mysql_free_result(res);
  
  sscanf(tmp, "%d.%d.%d", &major, &minor, &patch);

  check_server_version= major * 10000 + minor * 100 + patch;

  FAIL_IF(mysql_get_server_version(mysql) != check_server_version, "Numeric server version mismatch");
  FAIL_IF(strcmp(mysql_get_server_info(mysql), tmp) != 0, "String server version mismatch");
  return OK;
}

int test_conc26(MYSQL *my)
{
  MYSQL *mysql= mysql_init(NULL);
  mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8");

  FAIL_IF(mysql_real_connect(mysql, hostname, "notexistinguser", "password", schema, port, NULL, CLIENT_REMEMBER_OPTIONS), 
          "Error expected");

  FAIL_IF(!mysql->options.charset_name || strcmp(mysql->options.charset_name, "utf8") != 0, 
          "expected charsetname=utf8");
  mysql_close(mysql);

  mysql= mysql_init(NULL);
  FAIL_IF(mysql_real_connect(mysql, hostname, "notexistinguser", "password", schema, port, NULL, 0), 
          "Error expected");
  FAIL_IF(mysql->options.charset_name, "Error: options not freed");
  mysql_close(mysql);

  return OK;
}

int test_connection_timeout(MYSQL *my)
{
  unsigned int timeout= 5;
  time_t start, elapsed;
  MYSQL *mysql= mysql_init(NULL);
  mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, (unsigned int *)&timeout);
  start= time(NULL);
  if (mysql_real_connect(mysql, "192.168.1.101", "notexistinguser", "password", schema, port, NULL, CLIENT_REMEMBER_OPTIONS))
  {
    diag("Error expected - maybe you have to change hostname");
    return FAIL;
  }
  elapsed= time(NULL) - start;
  diag("elapsed: %lu", (unsigned long)elapsed);
  mysql_close(mysql);
  FAIL_IF(elapsed > 2 * timeout, "timeout ignored")
  return OK;
}

/* test should run with valgrind */
static int test_conc118(MYSQL *mysql)
{
  int rc;

  mysql->reconnect= 1;
  mysql->options.unused_1= 1;

  rc= mysql_kill(mysql, mysql_thread_id(mysql));
  sleep(2);

  rc= mysql_query(mysql, "SET @a:=1");
  check_mysql_rc(rc, mysql);

  FAIL_IF(mysql->options.unused_1 != 1, "options got lost");

  rc= mysql_kill(mysql, mysql_thread_id(mysql));
  sleep(2);

  mysql->host= "foo";

  rc= mysql_query(mysql, "SET @a:=1");
  FAIL_IF(!rc, "error expected");

  mysql->host= hostname;
  rc= mysql_query(mysql, "SET @a:=1");
  check_mysql_rc(rc, mysql);

  return OK;
}

struct my_tests_st my_tests[] = {
  {"test_conc118", test_conc118, TEST_CONNECTION_DEFAULT, 0, NULL,  NULL},
  {"test_conc66", test_conc66, TEST_CONNECTION_DEFAULT, 0, NULL,  NULL},
  {"test_bug20023", test_bug20023, TEST_CONNECTION_NEW, 0, NULL,  NULL},
  {"test_bug31669", test_bug31669, TEST_CONNECTION_NEW, 0, NULL,  NULL},
  {"test_bug33831", test_bug33831, TEST_CONNECTION_NEW, 0, NULL,  NULL},
  {"test_change_user", test_change_user, TEST_CONNECTION_NEW, 0, NULL,  NULL},
  {"test_opt_reconnect", test_opt_reconnect, TEST_CONNECTION_NONE, 0, NULL,  NULL},
  {"test_compress", test_compress, TEST_CONNECTION_NONE, 0, NULL,  NULL},
  {"test_reconnect", test_reconnect, TEST_CONNECTION_DEFAULT, 0, NULL, NULL},
  {"test_conc21", test_conc21, TEST_CONNECTION_DEFAULT, 0, NULL, NULL},
  {"test_conc26", test_conc26, TEST_CONNECTION_NONE, 0, NULL, NULL},
  {"test_connection_timeout", test_connection_timeout, TEST_CONNECTION_NONE, 0, NULL, NULL},
  {NULL, NULL, 0, 0, NULL, NULL}
};


int main(int argc, char **argv)
{
  if (argc > 1)
    get_options(argc, argv);

  get_envvars();

  run_tests(my_tests);

  return(exit_status());
}
