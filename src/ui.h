#ifndef CED_UI_H
#define CED_UI_H

#include "doc.h"
#include "common.h"

namespace ced::ui {

    // ##################################################################
    // function declarations
    // ##################################################################

    void init();
    void deinit();

    bool is_closed();

    void frame_begin();
    void frame_end();

    void panel(doc::iter_t*);

}

#endif // CED_UI_H
