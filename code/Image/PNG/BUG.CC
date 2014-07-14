//vxir-stone-68% /opt/cygnus/progressive-95q2/bin/g++ bug.cc
//bug.cc: In function `int main()':
//bug.cc:30: Internal compiler error.
//bug.cc:30: Please submit a Problem Report to Cygnus Support with send-pr.


#include <ostream.h>

class A
  {
  public:
  class B
    {
    public:
    B() { cout << "Constructor"; }
    int result;
    };

  B* B(void) { cout << "Function"; return(&_B); }
protected:
private:
  B _B;
  };
  
class C:public A
  {
  };


main()
{
C c;

cout << "Test:\n";

c.B();
}





