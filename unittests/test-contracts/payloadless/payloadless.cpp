#include "payloadless.hpp"

using namespace eosio;

void payloadless::doit() {
   print("Im a payloadless action");
}

EOSIO_DISPATCH( payloadless, (doit))
