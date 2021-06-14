#include "Value.h"
#include "Object.h"
#include <cmath>
#include <iostream>

namespace ash
{
	Value::Value()
		:type(VAL_NULL)
	{
		as.boolean = false;
	}

	Value::Value(bool value)
		: type(VAL_BOOL)
	{
		as.boolean = value;
	}

	Value::Value(int value)
		: type(VAL_INT)
	{
		as.integer = value;
	}

	Value::Value(float value)
		: type(VAL_FLOAT)
	{
		as.floating_point = value;
	}

	Value::Value(Object* value)
		: type(VAL_OBJ)
	{
		as.object = value;
	}

	void Value::print()
	{
		switch(type)
		{
		case VAL_NULL: std::cout << "null"; break;
		case VAL_BOOL: std::cout << (as_bool() ? "true" : "false"); break;
		case VAL_INT: std::cout << as_int(); break;
		case VAL_FLOAT: std::cout << as_float(); break;
		case VAL_OBJ: as_object()->print();
		}

		std::cout << '\n';
	}

	inline static bool bool_float(bool a, float b)
	{
		return a == (b != 0);
	}

	inline static bool bool_int(bool a, int b)
	{
		return a == (b != 0);
	}

	inline static bool int_float(int a, float b)
	{
		return (float)a == b;
	}

	bool Value::operator== (Value other)
	{
		switch (type)
		{
			case VAL_NULL: if (other.is_null()) return true;
			case VAL_BOOL:
			{
				switch (other.type)
				{
					case VAL_BOOL: return as_bool() == other.as_bool();
					case VAL_INT: return bool_int(as_bool(), other.as_int());
					case VAL_FLOAT: return bool_float(as_bool(), other.as_float());
				}
			}
			case VAL_INT:
			{
				switch (other.type)
				{
					case VAL_BOOL: return bool_int(other.as_bool(), as_int());
					case VAL_INT: return as_int() == other.as_int();
					case VAL_FLOAT: return int_float(as_int(), other.as_float());
				}
			}
			case VAL_FLOAT:
			{
				switch (other.type)
				{
					case VAL_BOOL: return bool_float(other.as_bool(), as_float());
					case VAL_INT: return int_float(other.as_int(), as_float());
					case VAL_FLOAT: return as_float() == other.as_float();
				}
			}
			case VAL_OBJ: if (other.is_object()) as_object() == other.as_object();
			default: return false;
		}
	}
}