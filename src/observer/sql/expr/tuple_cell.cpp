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
// Created by WangYunlai on 2022/07/05.
//

#include <sstream>
#include "sql/expr/tuple_cell.h"
#include "storage/field/field.h"
#include "common/log/log.h"
#include "common/lang/comparator.h"
#include "common/lang/string.h"
#include "sql/parser/parse_defs.h"

TupleCellSpec::TupleCellSpec(const char *table_name, const char *field_name, const char *alias, const AggrOp aggr)
{
  if (table_name) {
    table_name_ = table_name;
  }
  if (field_name) {
    field_name_ = field_name;
  }
  if(aggr){
    aggr_=aggr_to_string(aggr,aggr_);
  }
  if (alias) {
    alias_ = alias;
  } else {
    if (table_name_.empty()) {
      alias_ = field_name_;
    } else {
      alias_ = table_name_ + "." + field_name_;
    }
    if(aggr!=AggrOp::AGGR_NONE)
    {
      if (aggr == AggrOp::AGGR_COUNT_ALL)
         alias_ = "count(*)";
         else
         {
      std::string aggr_repr;
      aggr_repr = aggr_to_string(aggr,aggr_repr);
      alias_=aggr_repr+"("+alias_+")";
         }
    }
  }

}

TupleCellSpec::TupleCellSpec(const char *alias, const AggrOp aggr )
{
  if(aggr)
  {
    std::string aggr_repr_;
    aggr_=aggr_to_string(aggr,aggr_repr_);
  }
  if (alias) {
    alias_ = alias;
    if(aggr != AggrOp::AGGR_NONE)
    {
      if (aggr == AggrOp::AGGR_COUNT_ALL)
         alias_ = "count(*)";
         else
         {
           std::string aggr_repr;
           aggr_repr = aggr_to_string(aggr,aggr_repr);
           alias_=aggr_repr+"("+alias_+")";
      }
    }
  }
}

AggrOp string_to_aggr(const std::string& str)
  {
    if (str=="SUM") return AGGR_SUM;
    else if(str=="AVG") return AGGR_AVG;
    else if(str=="MAX") return AGGR_MAX;
    else if(str=="MIN") return AGGR_MIN;
    else if(str=="COUNT") return AGGR_COUNT;
    else if(str=="COUNT_ALL") return AGGR_COUNT_ALL;
    return AGGR_NONE;
  }

 //const std::string aggr_type_name[]={"sum","avg","max","min","count","count_all","none"};
 const std::string aggr_to_string(AggrOp aggr_type,std::string aggr_str)
 {
  if(aggr_type>=AGGR_SUM && aggr_type<=AGGR_COUNT_ALL)
  {
    aggr_str = aggr_type_name[aggr_type];
    return aggr_str;
  }
  else 
  {
    aggr_str = aggr_type_name[AGGR_NONE];
    return aggr_str;
  }
 }
