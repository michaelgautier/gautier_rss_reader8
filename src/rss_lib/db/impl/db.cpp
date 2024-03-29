/*
Copyright (C) 2020 Michael Gautier

This source code is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

This source code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with this library. If not, see <http://www.gnu.org/licenses/>.

Author: Michael Gautier <michaelgautier.wordpress.com>
*/

#include <iostream>
#include <sstream>
#include <string>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "rss_lib/db/db.hpp"

bool
gautier_rss_database::open_db (const std::string db_file_name, sqlite3** db)
{
	bool success = false;

	const int options = (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX);

	const int open_result = sqlite3_open_v2 (db_file_name.data(), db, options, nullptr);

	sqlite3_extended_result_codes (*db, 1);

	if (open_result == SQLITE_OK) {
		success = true;
	} else {
		std::cout << __FILE__ << " " << __func__ << " line (" << __LINE__ << ") Db Open Result: " << open_result <<
		          "\n\n";
	}

	return success;
}

void
gautier_rss_database::close_db (sqlite3** db)
{
	if (db) {
		sqlite3_close_v2 (*db);
	}

	return;
}

int
gautier_rss_database::create_sql_row (void* generic_object, const int col_count, char** col_values,
                                      char** col_names)
{
	sql_rowset_type* rows = (sql_rowset_type*)generic_object;

	rows->emplace_back (sql_row_type());

	sql_row_type* row = & (rows->back());

	for (int i = 0; i < col_count; i++) {
		const std::string name = col_names[i];
		const std::string value = (col_values[i] ? col_values[i] : "");

		row->insert_or_assign (name, value);
	}

	return 0;
}

void
gautier_rss_database::process_sql_simple (sqlite3** db, const std::string sql_text, sql_rowset_type& rows)
{
	sql_rowset_type* tmp_rows = &rows;

	char* error_message;

	sqlite3_exec (*db, sql_text.data(), create_sql_row, (void*)tmp_rows, &error_message);

	return;
}

void
gautier_rss_database::process_sql_simple (sqlite3** db, const std::string sql_text)
{
	char* error_message;

	sqlite3_exec (*db, sql_text.data(), nullptr, nullptr, &error_message);

	if (error_message) {
		std::cout << __FILE__ << " " << __func__ << " line (" << __LINE__ << ") SQL Result: " << error_message <<
		          "\n\n";
	}

	return;
}

void
gautier_rss_database::process_sql (sqlite3** db, const std::string sql_text,
                                   sql_parameter_list_type sql_param_values, sql_rowset_type& rows)
{
	sqlite3_stmt* sql_statement;

	int prepare_result = sqlite3_prepare_v2 (*db,  sql_text.data(),  -1,  &sql_statement,  nullptr);

	if (prepare_result != SQLITE_OK) {
		std::cout << __FILE__ << " " << __func__ << " line (" << __LINE__ << ") SQL Prepare Result: " << prepare_result
		          << "\n\n";
	}

	int i = 0;

	for (sql_parameter_list_type::value_type& sql_parameter : sql_param_values) {
		i++;
		sqlite3_bind_text (sql_statement, i, sql_parameter.data(), -1, SQLITE_TRANSIENT);
	}

	int sql_result = sqlite3_step (sql_statement);

	while (sql_result == SQLITE_OK || sql_result == SQLITE_ROW) {
		if (sql_result == SQLITE_OK || sql_result == SQLITE_ROW) {
			const int col_count = sqlite3_data_count (sql_statement);

			rows.emplace_back (sql_row_type());

			sql_row_type* row = & (rows.back());

			for (int col_i = 0; col_i < col_count; col_i++) {
				std::string col_name = sqlite3_column_name (sql_statement, col_i);

				std::stringstream col_value;

				col_value << sqlite3_column_text (sql_statement, col_i);

				row->insert_or_assign (col_name, col_value.str());
			}

			sql_result = sqlite3_step (sql_statement);
		} else if (sql_result == SQLITE_DONE) {
			sqlite3_reset (sql_statement);

			break;
		} else {
			const std::string sqlerror_message = sqlite3_errmsg (*db);

			if (sqlerror_message.empty() == false) {
				const std::string sqlerror_text = sqlite3_errstr (sql_result);

				std::cout << __FILE__ << " " << __func__ << " line (" << __LINE__ << ") MESSAGE: " << sqlerror_message << "\n";
				std::cout << "\t\t\t\t\t" << sqlerror_text << "\n";
			}
		}
	}

	sqlite3_finalize (sql_statement);

	return;
}

void
gautier_rss_database::process_sql (sqlite3** db, const std::string sql_text,
                                   sql_parameter_list_type sql_param_values)
{
	sql_rowset_type rows;

	process_sql (db, sql_text, sql_param_values, rows);

	return;
}
