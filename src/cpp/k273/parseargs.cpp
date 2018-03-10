
// local includes
#include "k273/parseargs.h"

#include "k273/strutils.h"
#include "k273/exception.h"


// std includes
#include <vector>
#include <string>

///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace K273;

///////////////////////////////////////////////////////////////////////////////

PargeArgs::PargeArgs(const vector<string>& args) :
    args(args),
    count(1) {
}

PargeArgs::~PargeArgs() {
}

int PargeArgs::more() {
    return this->args.size() - this->count;
}

bool PargeArgs::isNext(const std::string& next) {
    ASSERT (this->count < this->args.size());
    string cur = this->args[this->count];
    if (cur == next) {
        this->count++;
        return true;
    }

    return false;
}

const string& PargeArgs::getString() {
    ASSERT (this->count < this->args.size());
    return this->args[this->count++];
}

int PargeArgs::getInt() {
    return K273::toInt(this->args[this->count++]);
}
