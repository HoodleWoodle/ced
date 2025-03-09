#include "gui.h"

using namespace ced;

int main(int argc, char** argv) {
	gui::init();

	while (!gui::is_closed()) {
		gui::frame_begin();

		const char* text =
			"\n"
			"# build & run\n"
			"## ubuntusway\n"
			"```bash\n"
			"DIR_BUILD=.build \\\n"
			"	&& cmake -B $DIR_BUILD \\\n"
			"	&& make -j8 -C $DIR_BUILD \\\n"
			"	&& $DIR_BUILD/ced\n"
			"```"
		;
		gui::text(text);

		gui::frame_end();
	}

	gui::deinit();
	return 0;
}
