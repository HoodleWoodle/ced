#include "doc.h"
#include "ui.h"

using namespace ced;

int main(int argc, char** argv) {
	// TODO: IMPL: argument parsing
	const char* filepath = argv[1];

	doc::seg_t seg;
	doc::open(filepath, &seg);

	ui::init();
	while (!ui::is_closed()) {
		ui::frame_begin();

		doc::iter_t it;
		doc::iter(seg, &it);
		ui::panel(&it);

		ui::frame_end();
	}
	ui::deinit();

	doc::close(seg);
	return 0;
}
