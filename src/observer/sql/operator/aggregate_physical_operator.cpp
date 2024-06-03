#include "sql/operator/aggregate_physical_operator.h"
#include "common/log/log.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include <iostream>
#include <limits>
#include "sql/parser/value.h"
#include <iomanip>
#include <cstring>
#include <climits>

void AggregatePhysicalOperator::add_aggregation(AggrOp agg) { aggregations_.push_back(agg); }

RC AggregatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }
  std::unique_ptr<PhysicalOperator> &child = children_[0];
  RC                                 rc    = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  // trx_ = trx;

  return RC::SUCCESS;
}

RC AggregatePhysicalOperator::next()
{
  // already aggregated
  if (result_tuple_.cell_num() > 0) {
    return RC ::RECORD_EOF;
  }
  RC                 rc   = RC ::SUCCESS;
  PhysicalOperator  *oper = children_[0].get();
  int                opnumber = (int)aggregations_.size();
  std::vector<Value> result_cells(opnumber);
  int cell_flag = 0;
  
  // 先在此处对result_cells进行初始化，后续功能自行添加
  for(int i = 0; i < opnumber; i++) 
  {
    const AggrOp aggregation = aggregations_[i];
    if (aggregation == AggrOp::AGGR_SUM) 
    {
      result_cells[i].set_type(AttrType::FLOATS);
      result_cells[i].set_float(0.0);
    }
  }

  std::vector<Value> sum_cells(opnumber);
          for(int i = 0; i < opnumber; i++) 
          {
            const AggrOp aggregation_ = aggregations_[i];    
            if(aggregation_ == AggrOp::AGGR_AVG)   
            {
            sum_cells[i].set_type(AttrType::FLOATS);
            sum_cells[i].set_float(0.0);   
            }        
          }
  std::vector<int> count_cells(aggregations_.size(),0);

  std::vector<double> max_cells(aggregations_.size(),-std::numeric_limits<double>::infinity());
  std::vector<string> max_cells_(opnumber,"");

  std::vector<double> min_cells(aggregations_.size(),std::numeric_limits<double>::infinity());
  std::vector<string> min_cells_(opnumber,"");

  std::string result_string;

  int my_index = 0;
          
  while (RC ::SUCCESS == (rc = oper->next())) {
    // get tuple
    Tuple *tuple = oper->current_tuple();

    // do aggregate
    for (int cell_idx = 0; cell_idx < (int)aggregations_.size(); cell_idx++) {
      const AggrOp aggregation = aggregations_[cell_idx];
      Value        cell;
      AttrType     attr_type = AttrType::INTS;
      switch (aggregation) {
        case AggrOp::AGGR_SUM:
          rc        = tuple->cell_at(cell_idx, cell);
          attr_type = cell.attr_type();
          if (attr_type == AttrType ::INTS or attr_type == AttrType ::FLOATS) {
            result_cells[cell_idx].set_float(result_cells[cell_idx].get_float() + cell.get_float());
          }
          break;

          case AggrOp::AGGR_AVG:
          rc        = tuple->cell_at(cell_idx, cell);
          attr_type = cell.attr_type();
         // cell_flag = 1;
          if (attr_type == AttrType ::INTS or attr_type == AttrType ::FLOATS)
          {
            sum_cells[cell_idx].set_float(sum_cells[cell_idx].get_float() + cell.get_float());  
            count_cells[cell_idx]++;  
            
          }
          break;

          case AggrOp::AGGR_MAX:
          rc        = tuple->cell_at(cell_idx, cell);
          attr_type = cell.attr_type();
        //  cell_flag = 2;
          if (attr_type == AttrType ::INTS or attr_type == AttrType ::FLOATS )
          {
            double cell_value = (attr_type == AttrType::INTS) ? static_cast<double>(cell.get_int()) : cell.get_float();
            if (cell_value > max_cells[cell_idx])
            {
              max_cells[cell_idx] = cell_value;
            }
          }
           else if (attr_type == AttrType::CHARS) {  
            cell_flag = 1;
            std::string cell_value = cell.get_string();  
            if (cell_value > max_cells_[cell_idx]) {  
                max_cells_[cell_idx] = cell_value;  
             }  
           }  
          break;

          case AggrOp::AGGR_MIN:
          rc        = tuple->cell_at(cell_idx, cell);
          attr_type = cell.attr_type();
          //cell_flag = 3;
          if (attr_type == AttrType ::INTS or attr_type == AttrType ::FLOATS )
          {
            double cell_value = (attr_type == AttrType::INTS) ? static_cast<double>(cell.get_int()) : cell.get_float();
            if (cell_value < min_cells[cell_idx])
            {
              min_cells[cell_idx] = cell_value;
            }
          }
           else if (attr_type == AttrType::CHARS) {  
            cell_flag = 1;
             std::string cell_value = cell.get_string();  
             if (my_index == 0)
             {
              min_cells_[my_index] = cell_value;  
             }
            if (cell_value < min_cells_[cell_idx] ) {  
                min_cells_[cell_idx] = cell_value;  
             }  
           } 
          break;

          case AggrOp::AGGR_COUNT:
          rc        = tuple->cell_at(cell_idx, cell);
          attr_type = cell.attr_type();
          //cell_flag = 4;
          count_cells[cell_idx]++;  
          break;

          case AggrOp::AGGR_COUNT_ALL:
          rc        = tuple->cell_at(cell_idx, cell);
          attr_type = cell.attr_type();
          //cell_flag = 4;
          count_cells[cell_idx]++;  
          break;

        default: return RC ::UNIMPLEMENT;
      }
    }
    my_index++;
  }
  //if(cell_flag == 1)
  {
    for (int cell_idx = 0; cell_idx < (int)aggregations_.size(); cell_idx++) {  
                if (count_cells[cell_idx] > 0) 
                {  
                     result_cells[cell_idx].set_float(sum_cells[cell_idx].get_float() / count_cells[cell_idx]);  
                }
               }
  }
  //if (cell_flag == 2)
  {
    for (int cell_idx = 0; cell_idx < (int)aggregations_.size(); cell_idx++) {  
    if (aggregations_[cell_idx] == AggrOp::AGGR_MAX) {  
        const char *sp_cell = max_cells_[cell_idx].c_str();
        if (cell_flag)
        {
          result_cells[cell_idx].set_string(sp_cell);  
        }
        else
        result_cells[cell_idx].set_float(max_cells[cell_idx]);  
       }  
   }
  }

  //if (cell_flag == 3)
  {
    for (int cell_idx = 0; cell_idx < (int)aggregations_.size(); cell_idx++) {  
    if (aggregations_[cell_idx] == AggrOp::AGGR_MIN) {  
        const char *sp_cell = min_cells_[cell_idx].c_str();
        if (cell_flag)
        {
          result_cells[cell_idx].set_string(sp_cell);  
        }
        else
        result_cells[cell_idx].set_float(min_cells[cell_idx]);  
       }  
   }
  }

  //if (cell_flag == 4)
  {
    for (int cell_idx = 0; cell_idx < (int)aggregations_.size(); cell_idx++) {  
    if (aggregations_[cell_idx] == AggrOp::AGGR_COUNT or aggregations_[cell_idx] == AggrOp::AGGR_COUNT_ALL) {  
        result_cells[cell_idx].set_float(count_cells[cell_idx]);  
       }  
   }
  }

  if (rc == RC ::RECORD_EOF) {
    rc = RC ::SUCCESS;
  }
  result_tuple_.set_cells(result_cells);
  return rc;
}

RC AggregatePhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}
