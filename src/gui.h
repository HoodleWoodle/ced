#ifndef CED_GUI_H
#define CED_GUI_H

namespace ced::gui {
    void init();
    void deinit();

    bool is_closed();

    void frame_begin();
    void frame_end();

    void text(const char*);
}

#endif // CED_GUI_H
