#include <stdio.h>
#include "engine/engine.h"

int main()
{
	Engine engine = {0};
	printf("%lu e\n", sizeof(Engine));

    engine_initialise(&engine);

    engine_run(&engine);

    engine_cleanup(&engine);
}

