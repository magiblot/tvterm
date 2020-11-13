#include <tvterm/vtermview.h>
#include <tvterm/vtermwnd.h>
#include <vterm.h>

TVTermView::TVTermView(const TRect &bounds, TVTermWindow &window) :
    TView(bounds),
    window(window),
    vterm(*this)
{
    growMode = gfGrowHiX | gfGrowHiY;
    options |= ofSelectable | ofFirstClick;
    showCursor();
}

const VTermScreenCallbacks TVTermAdapter::callbacks = {
    &damage,
    &moverect,
    &movecursor,
    &settermprop,
    &bell,
    &resize,
    &sb_pushline,
    &sb_popline,
};

TVTermAdapter::TVTermAdapter(TVTermView &view) :
    view(view)
{
    auto size = view.size;
    vt = vterm_new(size.y, size.x);
    vterm_set_utf8(vt, 1);

    vts = vterm_obtain_screen(vt);
    vterm_screen_set_callbacks(vts, &callbacks, this);
}

TVTermAdapter::~TVTermAdapter()
{
    vterm_free(vt);
}

int TVTermAdapter::damage(VTermRect rect, void *user)
{
    auto *self = (TVTermAdapter *) user;
    return 0;
}

int TVTermAdapter::moverect(VTermRect dest, VTermRect src, void *user)
{
    auto *self = (TVTermAdapter *) user;
    return 0;
}

int TVTermAdapter::movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
    auto *self = (TVTermAdapter *) user;
    return 0;
}

int TVTermAdapter::settermprop(VTermProp prop, VTermValue *val, void *user)
{
    auto *self = (TVTermAdapter *) user;
    return 0;
}

int TVTermAdapter::bell(void *user)
{
    auto *self = (TVTermAdapter *) user;
    return 0;
}

int TVTermAdapter::resize(int rows, int cols, void *user)
{
    auto *self = (TVTermAdapter *) user;
    return 0;
}

int TVTermAdapter::sb_pushline(int cols, const VTermScreenCell *cells, void *user)
{
    auto *self = (TVTermAdapter *) user;
    return 0;
}

int TVTermAdapter::sb_popline(int cols, VTermScreenCell *cells, void *user)
{
    auto *self = (TVTermAdapter *) user;
    return 0;
}
