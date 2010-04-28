#include "Mem.h"
#include "Object.h"
#include <stdio.h>
#ifdef	LINUX
	#include <unistd.h>
#endif
#include <vector>
#include "../r_code/utils.h"
#include "hash_containers"

namespace r_exec {

using namespace std;

using r_code::Atom;

struct TestReceiver : public ObjectReceiver
{
	virtual void receive(
		Object *object,
		std::vector<Atom> viewData,
		int node_id,
		Destination dest
	) {
		printf("got object %p\n", object);
	}
		
	void beginBatchReceive() {}
	void endBatchReceive() {}
};

void read_object(char* fileName, vector<Object*>& objects, vector<vector<Atom> >& views)
{
	FILE* fp = fopen(fileName, "r");
	vector<Atom> viewData;
	vector<Object*> references;
	vector<Atom> objectAtoms;
	char buf[1000];
	for (int i = 0; ; ++i) {
		if (!fgets(buf, sizeof(buf), fp)) {
			views.push_back(viewData);
			Object* obj = Object::create(objectAtoms, references);
			objects.push_back(obj);
			return;
		}
		if (i < 3) {
			float viewValue;
			if (sscanf(buf, "%f", &viewValue) == 1) {
				viewData.push_back(Atom::Float(viewValue));
			} else {
				fprintf(stderr, "didn't parse %s", buf);
			}
		} else {
			int atom;
			int refNumber;
			if (sscanf(buf, "%*d: [%x]", &atom) == 1) {
				objectAtoms.push_back(Atom(atom));
			} else if (sscanf(buf, "%d", &refNumber) == 1) {
				references.push_back(objects[refNumber]);
			} else {
				fprintf(stderr, "didn't parse %s", buf);
			}
		}
	}
}

}

using namespace r_exec;

int main(int argc, char** argv)
{
	TestReceiver test;
	Mem* mem = Mem::create(10000,10000,UNORDERED_MAP<string, r_code::Atom>(), vector<r_code::Object*>(), &test);
	vector<Object*> objects;
	vector<vector<Atom> > views;
	for (int i = 1; i < argc; ++i)
	{
		read_object(argv[i], objects, views);
		mem->receive(objects[i-1], views[i-1], 1, ObjectReceiver::INPUT_GROUP);
	}
	r_code::Thread::Sleep(10);
}

