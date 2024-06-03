#include "common/log/log.h"
#include "sql/operator/update_physical_operator.h"
#include "sql/stmt/insert_stmt.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include "sql/stmt/update_stmt.h"
#include "storage/trx/trx.h"
using namespace std;

UpdatePhysicalOperator::UpdatePhysicalOperator(Table *table, Field field, Value value)
    : table_(table), field_(field), value_(value)
{}


RC UpdatePhysicalOperator::open(Trx *trx)
{
  
  if (children_.empty()) {
    return RC::SUCCESS;
  }
  std::unique_ptr<PhysicalOperator> &child = children_[0];
  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  trx_ = trx;

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next()
{
  RC rc = RC::SUCCESS;
  if (children_.empty()) {
    return RC::RECORD_EOF;
  }

  PhysicalOperator *child = children_[0].get();

  std::vector<Record> insert_records; // 用于存储要插入的记录
  std::vector<Record> delete_records; // 用于存储要删除的记录
  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record &record = row_tuple->record();

    delete_records.emplace_back(record);//把这个记录写入要删的记录里面
    RC rc=RC::SUCCESS;
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to delete record: %s", strrc(rc));
      return rc;
    }else
    {
        //定位列名索引
        const std::vector<FieldMeta> *table_field_metas=table_->table_meta().field_metas();
        const char *target_field_name=field_.field_name();

        int meta_num=table_field_metas->size();
        int target_index=-1;
        for(int i=0;i<meta_num;++i)
        {
            FieldMeta fieldmeta=(*table_field_metas)[i];
            const char *field_name=fieldmeta.name();
            if(0==strcmp(field_name,target_field_name)){
                target_index=i;
                break;

            }

        }
        //重新构造record
        //1.Values
        std::vector<Value> values;
        int cell_num=row_tuple->cell_num();
        for(int i=0;i<cell_num;i++)
        {
            Value cell;
            if(target_index!=i)
            {
                row_tuple->cell_at(i,cell);//如果当前单元不是目标字段的索引,从原始记录中获取第 i 个单元的值，并将其存储到 cell 中。
            }
            else
            {
                cell.set_value(value_);//如果当前单元是目标字段的索引，将目标字段的值 value_ 设置到 cell 中。从而完成把想要设置的值设置进去的目的
            }
            values.emplace_back(cell);
        }
        //2.records
        Record new_record;
        RC rc = table_->make_record(cell_num, values.data(), new_record);//制造一条新纪录，传参属性个数，改后的值，空的新纪录
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to make record. rc=%s", strrc(rc));
          return rc;
        }

      insert_records.emplace_back(new_record);//把这个记录写入要插的记录里面

    }

  }
  for (size_t i=0;i<insert_records.size();++i){//遍历每个要插的记录
    rc = trx_->delete_record(table_, delete_records[i]);//删除之前存在delete_records里面的要删的记录
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to delete record: %s", strrc(rc));
      return rc;
    }
    rc = trx_->insert_record(table_, insert_records[i]);//插入之前存在insert_records里面的要删的记录
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to insert record: %s", strrc(rc));
      return rc;
    }
  }
  return RC::RECORD_EOF;
}

RC UpdatePhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}