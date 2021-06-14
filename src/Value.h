#pragma once
namespace ash
{
	enum ValueType
	{
		VAL_BOOL,
		VAL_NULL,
		VAL_INT,
		VAL_FLOAT,
		VAL_OBJ
	};

	class Object;

	union _AS
	{
		bool boolean;
		int integer;
		float floating_point;
		Object* object;
	};

	class Value
	{
	private:
		ValueType type;
		_AS as;

	public:
		Value();
		Value(bool value);
		Value(int value);
		Value(float value);
		Value(Object* value);

		void print();

		bool is_null() { return type == VAL_NULL; };
		bool is_bool() { return type == VAL_BOOL; };
		bool is_int() { return type == VAL_INT; };
		bool is_float() { return type == VAL_FLOAT; };
		bool is_object() { return type == VAL_OBJ; };

		bool as_bool() { return as.boolean; }
		int as_int() { return as.integer; }
		float as_float() { return as.floating_point; }
		Object* as_object() { return as.object; }

		bool isFalsey() { return is_null() 
			|| (is_bool() && as_bool()) 
			|| (is_int() && as_int() == 0); }

		bool operator== (Value other);
	};
}