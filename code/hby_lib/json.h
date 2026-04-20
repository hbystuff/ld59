#pragma once

// Note: My own json thing isn't implemented yet.

//====================================================================
// JSON module begin
typedef enum Json_Value_Type {
  JSON_VALUE_TYPE_NUMBER,
  JSON_VALUE_TYPE_OBJECT,
  JSON_VALUE_TYPE_ARRAY,
  JSON_VALUE_TYPE_TRUE,
  JSON_VALUE_TYPE_FALSE,
  JSON_VALUE_TYPE_STRING,
} Json_Value_Type;

typedef struct Json_Value {
  char *name;
  Json_Value_Type   type;
  struct Json_Value *next;
  union {
    double             number_;
    char              *string_;
    struct Json_Field *object_;
    struct Json_Field *array_;
  };
} Json_Value;

// JSON module end
//====================================================================

