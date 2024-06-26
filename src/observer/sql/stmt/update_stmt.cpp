/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/5/22.
//

#include "sql/stmt/update_stmt.h"
#include "common/log/log.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include <string>
#include "common/lang/string.h"
#include "sql/parser/parse_defs.h"

UpdateStmt::UpdateStmt(Table *table, Field field, Value value, FilterStmt *filter_stmt)
    : table_(table), field_(field), value_(value), filter_stmt_(filter_stmt)
{}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt)
{
  if (db == nullptr) {
    LOG_WARN("invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }

  const char *table_name = update_sql.relation_name.c_str();

  if (table_name == nullptr) {
    LOG_WARN("invalid argument. relation name is null.");
    return RC::INVALID_ARGUMENT;
  }

  Table *table = db->find_table(table_name);

  if (table == nullptr) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  const FieldMeta *filed_meta = table->table_meta().field(update_sql.attribute_name.c_str());

  if (filed_meta == nullptr) {
    LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), update_sql.attribute_name.c_str());
    return RC::SCHEMA_FIELD_MISSING;
  }

  for (auto condition : update_sql.conditions) {
    const FieldMeta *condition_field_meta = table->table_meta().field(condition.left_attr.attribute_name.c_str());
    if (nullptr == condition_field_meta) {
      LOG_WARN("no such field in condition. field=%s.%s.%s",db->name(),table->name(),condition.left_attr.attribute_name.c_str());
      return RC::SCHEMA_FIELD_MISSING;
    }
  }

  std::unordered_map<std::string, Table *> table_map;
  table_map.insert(std::pair<std::string, Table *>(table_name, table));
  FilterStmt *filter_stmt = nullptr;
  RC          rc          = FilterStmt::create(
      db, table, &table_map, update_sql.conditions.data(), static_cast<int>(update_sql.conditions.size()), filter_stmt);

  if (rc != RC::SUCCESS) {
    LOG_WARN("cannot construct filter stmt");
    return rc;
  }

  UpdateStmt *update_stmt = new UpdateStmt(table, Field(table, filed_meta), update_sql.value, filter_stmt);

  stmt = update_stmt;
  return RC::SUCCESS;
}
