#include "engine/engine.h"

int main()
{
	Engine engine;

    engine_initialise(&engine);

    engine_run(&engine);

    engine_cleanup(&engine);
}

