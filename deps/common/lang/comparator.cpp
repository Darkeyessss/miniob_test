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
// Created by wangyunlai on 2021/6/11.
//

#include "common/defs.h"
#include <algorithm>
#include <string.h>
#include <stdio.h>  
#include <stdlib.h>  
#include <stdbool.h>  
#include <climits> 

bool string_to_int1(const char *str, int *result) {  
    char *endptr;  
    long num = strtol(str, &endptr, 10); // 使用strtol来避免整数溢出  
  
    // 检查转换是否成功（endptr应该指向字符串的末尾）  
    if ((endptr == str) || (*endptr != '\0') || (errno == ERANGE) ||  
        (num > INT_MAX) || (num < INT_MIN)) {  
        return false;  
    }  
    *result = (int)num;  
    return true;  
}  

namespace common {

int compare_int(void *arg1, void *arg2)
{
  int v1 = *(int *)arg1;
  int v2 = *(int *)arg2;
  if (v1 > v2) {
    return 1;
  } else if (v1 < v2) {
    return -1;
  } else {
    return 0;
  }
}

int compare_float(void *arg1, void *arg2)
{
  float v1  = *(float *)arg1;
  float v2  = *(float *)arg2;
  float cmp = v1 - v2;
  if (cmp > EPSILON) {
    return 1;
  }
  if (cmp < -EPSILON) {
    return -1;
  }
  return 0;
}

int compare_string(void *arg1, int arg1_max_length, void *arg2, int arg2_max_length)
{
  const char *s1     = (const char *)arg1;
  const char *s2     = (const char *)arg2;
  int         maxlen = std::min(arg1_max_length, arg2_max_length);
  int         result = strncmp(s1, s2, maxlen);
  if (0 != result) {
    return result;
  }

  if (arg1_max_length > maxlen) {
    return s1[maxlen] - 0;
  }

  if (arg2_max_length > maxlen) {
    return 0 - s2[maxlen];
  }
  return 0;
}
int compare_date(void* arg1,void* arg2)
{
  return compare_int(arg1,arg2);
}
}  // namespace common
int compare_str_with_int(void *arg1, int arg1_max_length, void *arg2)
{
    char *str = (char *)arg1;  
    int str_int;  
    string_to_int1(str, &str_int);
    int *a2 = (int *)arg2;
    // 现在我们可以比较两个整数了  
    if (str_int > *a2) {  
        return 1; // 字符串表示的整数大于给定的整数  
    } else if (str_int < *a2) {  
        return -1; // 字符串表示的整数小于给定的整数  
    } else {  
        return 0; // 字符串表示的整数等于给定的整数  
    }  
}
int compare_str_with_float(void *arg1, int arg1_max_length, void *arg2)
{
   char *str = (char *)arg1;  
    float *num =(float *)arg2;  
  
    // 使用 atof 函数将字符串转换为浮点数，并与给定的浮点数进行比较  
    float str_num = atof(str);  
    return (str_num > *num) - (str_num < *num); // 如果 str_num > num，返回 1；如果 str_num < num，返回 -1；否则返回 0  
}

