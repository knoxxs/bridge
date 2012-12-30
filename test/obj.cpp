#include <iostream>
using namespace std;

class class1{
		
	public:
		int i;
		class1(int a)
		{
			i=a;
		}
		class1()
		{

		}
		int geti()
		{
			cout<<"value in class1  get"<<i <<endl;
			return i;
		}
};

class class2{
private:
	class1 obj;
public:
	class2(class1 o)
	{
		obj = o;
	}
	int get(){
		cout<< "value in class2 get"<<obj.geti();
		return obj.geti();
	}
	int change;
};

void func(class1* a){
	a->geti();
	a->i = 10;
	a->geti();
}

void func2(class1& a){
	a.geti();
	a.i = 100;
	a.geti();
}

int main()
{
	int i =1;
	class1 o1(1);
	o1.geti();

	func(&o1);
	func2(o1);

	o1.geti();

	// class2 o2(o1);
	// o2.get();
	return 0;
}