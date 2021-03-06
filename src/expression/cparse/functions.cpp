#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <stdexcept>
#include <cerrno>
#include <iostream>
#include <cctype>  // For tolower() and toupper()

#include "./shunting_yard.h"
#include "./functions.h"


/* * * * * class Function * * * * */
packToken Function::call(packToken _this, Function* func,
                         Tuple* args, TokenMap scope) {
  // Build the local namespace:
  TokenMap local = scope.getChild();

  // Add args to local namespace:
  for (const std::string& name : func->args()) {
    packToken value;
    if (args->size()) {
      value = packToken(args->pop_front());
    } else {
      value = packToken::None;
    }

    local[name] = value;
  }

  TokenList arglist;
  // Collect any extra arguments:
  while (args->size()) {
    arglist.list().push_back(packToken(args->pop_front()));
  }
  local["args"] = arglist;
  local["this"] = _this;

  return func->exec(local);
}

/* * * * * User defined Functions: * * * * */
const char* cron_args[] = {"arg1", "arg2"};
packToken cron(TokenMap scope) {
  // Get two arguments:
  int arg1 = scope.find("arg1")->asInt();
  int arg2 = scope.find("arg2")->asInt();
  if (arg1==arg2) return 1;
  else return 0;
}

const char* max_args[] = {"arg1", "arg2"};
packToken max(TokenMap scope) {
  // Get two arguments:
  double arg1 = scope.find("arg1")->asDouble();
  double arg2 = scope.find("arg2")->asDouble();
  if (arg1>=arg2) return arg1;
  else return arg2;
}

const char* min_args[] = {"arg1", "arg2"};
packToken min(TokenMap scope) {
  // Get two arguments:
  double arg1 = scope.find("arg1")->asDouble();
  double arg2 = scope.find("arg2")->asDouble();
  if (arg1<=arg2) return arg1;
  else return arg2;
}


/* * * * * Built-in Functions: * * * * */

packToken default_print(TokenMap scope) {
  // Get the argument list:
  TokenList list = scope.find("args")->asList();

  bool first = true;
  for (packToken item : list.list()) {
    if (first) {
      first = false;
    } else {
      std::cout << " ";
    }

    if (item->type == STR) {
      std::cout << item.asString();
    } else {
      std::cout << item.str();
    }
  }

  std::cout << std::endl;

  return packToken::None;
}

packToken default_sum(TokenMap scope) {
  // Get the arguments:
  TokenList list = scope.find("args")->asList();

  if (list.list().size() == 1 && list.list().front()->type == LIST) {
    list = list.list().front().asList();
  }

  double sum = 0;
  for (packToken num : list.list()) {
    sum += num.asDouble();
  }

  return sum;
}

const char* value_arg[] = {"value"};
packToken default_eval(TokenMap scope) {
  std::string code = scope.find("value")->asString();
  // Evaluate it as a calculator expression:
  return calculator::calculate(code.c_str(), scope);
}

packToken default_float(TokenMap scope) {
  packToken* tok = scope.find("value");
  if ((*tok)->type & NUM) return tok->asDouble();

  // Convert it to double:
  char* rest;
  const std::string& str = tok->asString();
  errno = 0;
  double ret = strtod(str.c_str(), &rest);

  if (str == rest) {
    throw std::runtime_error("Could not convert \"" + str + "\" to float!");
  } else if (errno) {
    std::range_error("Value too big or too small to fit a Double!");
  }
  return ret;
}

packToken default_int(TokenMap scope) {
  packToken* tok = scope.find("value");
  if ((*tok)->type & NUM) return tok->asInt();

  // Convert it to double:
  char* rest;
  const std::string& str = tok->asString();
  errno = 0;
  int64_t ret = strtol(str.c_str(), &rest, 10);

  if (str == rest) {
    throw std::runtime_error("Could not convert \"" + str + "\" to integer!");
  } else if (errno) {
    std::range_error("Value too big or too small to fit an Integer!");
  }
  return ret;
}

packToken default_str(TokenMap scope) {
  // Return its string representation:
  packToken* tok = scope.find("value");
  if ((*tok)->type == STR) return *tok;
  return tok->str();
}

packToken default_type(TokenMap scope) {
  packToken* tok = scope.find("value");
  switch ((*tok)->type) {
  case NONE: return "none";
  case VAR: return "variable";
  case REAL: return "float";
  case INT: return "integer";
  case STR: return "string";
  case FUNC: return "function";
  case IT: return "iterable";
  case TUPLE: return "tuple";
  case LIST: return "list";
  case MAP: return "map";
  default: return "unknown_type";
  }
}

packToken default_extend(TokenMap scope) {
  packToken* tok = scope.find("value");

  if ((*tok)->type == MAP) {
    return tok->asMap().getChild();
  } else {
    throw std::runtime_error(tok->str() + " is not extensible!");
  }
}

packToken default_instanceof(TokenMap scope) {
  TokenMap _super = scope.find("value")->asMap();
  TokenMap* _this = scope.find("this")->asMap().parent();

  TokenMap* parent = _this;
  while (parent) {
    if ((*parent) == _super) {
      return true;
    }

    parent = parent->parent();
  }

  return false;
}

const char* num_arg[] = {"number"};
packToken default_sqrt(TokenMap scope) {
  // Get a single argument:
  double number = scope.find("number")->asDouble();

  return sqrt(number);
}
packToken default_sin(TokenMap scope) {
  // Get a single argument:
  double number = scope.find("number")->asDouble();

  return sin(number);
}
packToken default_cos(TokenMap scope) {
  // Get a single argument:
  double number = scope.find("number")->asDouble();

  return cos(number);
}
packToken default_tan(TokenMap scope) {
  // Get a single argument:
  double number = scope.find("number")->asDouble();

  return tan(number);
}
packToken default_abs(TokenMap scope) {
  // Get a single argument:
  double number = scope.find("number")->asDouble();

  return std::abs(number);
}

const char* pow_args[] = {"number", "exp"};
packToken default_pow(TokenMap scope) {
  // Get two arguments:
  double number = scope.find("number")->asDouble();
  double exp = scope.find("exp")->asDouble();

  return pow(number, exp);
}

/* * * * * Type-specific default functions * * * * */

packToken string_len(TokenMap scope) {
  std::string str = scope.find("this")->asString();
  return static_cast<int64_t>(str.size());
}

packToken string_lower(TokenMap scope) {
  std::string str = scope.find("this")->asString();
  std::string out;
  for (char c : str) {
    out.push_back(tolower(c));
  }
  return out;
}

packToken string_upper(TokenMap scope) {
  std::string str = scope.find("this")->asString();
  std::string out;
  for (char c : str) {
    out.push_back(toupper(c));
  }
  return out;
}

/* * * * * default constructor functions * * * * */

packToken default_list(TokenMap scope) {
  // Get the arguments:
  TokenList list = scope.find("args")->asList();

  // If the only argument is iterable:
  if (list.list().size() == 1 && list.list()[0]->type & IT) {
    return TokenList(list.list()[0]);
  } else {
    return list;
  }
}

packToken default_map(TokenMap scope) {
  return TokenMap();
}

/* * * * * class CppFunction * * * * */

CppFunction::CppFunction(packToken (*func)(TokenMap), unsigned int nargs,
                         const char** args, std::string name)
                         : func(func) {
  this->name = name;
  // Add all strings to args list:
  for (uint32_t i = 0; i < nargs; ++i) {
    this->_args.push_back(args[i]);
  }
}

// Build a function with no named args:
CppFunction::CppFunction(packToken (*func)(TokenMap), std::string name)
                         : func(func) {
  this->name = name;
}

/* * * * * CppFunction Initializer Constructor: * * * * */

struct CppFunction::Startup {
  Startup() {
    TokenMap& global = TokenMap::default_global();

    global["print"] = CppFunction(&default_print, "print");
    global["sum"] = CppFunction(&default_sum, "sum");
    global["sqrt"] = CppFunction(&default_sqrt, 1, num_arg, "sqrt");
    global["sin"] = CppFunction(&default_sin, 1, num_arg, "sin");
    global["cos"] = CppFunction(&default_cos, 1, num_arg, "cos");
    global["tan"] = CppFunction(&default_tan, 1, num_arg, "tan");
    global["abs"] = CppFunction(&default_abs, 1, num_arg, "abs");
    global["pow"] = CppFunction(&default_pow, 2, pow_args, "pow");
    global["float"] = CppFunction(&default_float, 1, value_arg, "float");
    global["str"] = CppFunction(&default_str, 1, value_arg, "str");
    global["eval"] = CppFunction(&default_eval, 1, value_arg, "eval");
    global["type"] = CppFunction(&default_type, 1, value_arg, "type");
    global["extend"] = CppFunction(&default_extend, 1, value_arg, "extend");
    global["cron"] = CppFunction(&cron, 2, cron_args, "cron");
    global["max"] = CppFunction(&max, 2, max_args, "max");
    global["min"] = CppFunction(&min, 2, min_args, "min");

    // Default constructors:
    global["list"] = CppFunction(&default_list, "list");
    global["map"] = CppFunction(&default_map, "map");

    TokenMap& base_map = TokenMap::base_map();
    base_map["instanceof"] = CppFunction(&default_instanceof, 1,
                                         value_arg, "instanceof");

    typeMap_t& type_map = calculator::type_attribute_map();
    type_map[STR]["len"] = CppFunction(&string_len, "len");
    type_map[STR]["lower"] = CppFunction(&string_lower, "lower");
    type_map[STR]["upper"] = CppFunction(&string_upper, "upper");
  }
} CppFunction_startup;

