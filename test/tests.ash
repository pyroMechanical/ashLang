multi
int test(int x)
{
   int i = x
   x = i + 1
   return x
}

int i = test(0)
i = test(i)
exec

multi
int fib(int x)
{
    if(x <= 1)
    {
        return x
    }
    else
    {
        return fib(x - 1) + fib(x - 2)
    }
}

int i = fib(25)
exec

multi
def foo { float f }
def bar { foo _foo }
def quux { bar _bar }
quux test = { { { 25.1f } } }

float unpackQuux(quux q)
{
	return q._bar._foo.f
}

unpackQuux(test)
exec

multi
def Integer { int value }

Integer decrement(Integer x)
{
    if(x.value <= 0)
    {
        return x
    }
    else
    {
        return decrement({x.value - 1})
    }
}

Integer k = decrement({15})
exec

multi
def Integer { int value }

Integer fibI(Integer x)
{
   if(x.value <= 1)
   {
      return x
   }
   else
   {
	  Integer result =  { fibI({x.value - 1}).value + fibI({x.value - 2}).value }
      return result
   }
}

Integer i = { 15 }
i = fibI(i)
exec
