#include <optional>
#include <variant>

extern "C" {
#include <X11/X.h>
#include <X11/Xlib.h>

#ifdef BUILD_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif /* BUILD_XDAMAGE */
}

#include "x11-event.h"
#include "x11.h"

namespace conky::x11 {
event::event(Display* display, XEvent ev) {
#define BIND(type_value, field) \
  if (ev.type == type_value) {  \
    inner = ev.field;           \
    return;                     \
  }

  // BIND(KeyPress, xkey)         // Use XInput2!
  // BIND(KeyRelease, xkey)       // Use XInput2!
  // BIND(ButtonPress, xbutton)   // Use XInput2!
  // BIND(ButtonRelease, xbutton) // Use XInput2!
  // BIND(MotionNotify, xmotion)  // Use XInput2!
  // BIND(EnterNotify, xcrossing) // Use XInput2!
  // BIND(LeaveNotify, xcrossing) // Use XInput2!
  // BIND(FocusIn, xfocus)        // Use XInput2!
  // BIND(FocusOut, xfocus)       // Use XInput2!
  // BIND(KeymapNotify, xkeymap)  // Use XInput2!
  BIND(Expose, xexpose)
  // BIND(GraphicsExpose, xgraphicsexpose)
  // BIND(NoExpose, xnoexpose)
  // BIND(VisibilityNotify, xvisibility)
  // BIND(CreateNotify, xcreatewindow)
  // BIND(DestroyNotify, xdestroywindow)
  // BIND(UnmapNotify, xunmap)
  // BIND(MapNotify, xmap)
  // BIND(MapRequest, xmaprequest)
  BIND(ReparentNotify, xreparent)
  BIND(ConfigureNotify, xconfigure)
  // BIND(ConfigureRequest, xconfigurerequest)
  // BIND(GravityNotify, xgravity)
  // BIND(ResizeRequest, xresizerequest)
  // BIND(CirculateNotify, xcirculate)
  // BIND(CirculateRequest, xcirculaterequest)
  BIND(PropertyNotify, xproperty)
  // BIND(SelectionClear, xselectionclear)
  // BIND(SelectionRequest, xselectionrequest)
  // BIND(SelectionNotify, xselection)
  // BIND(ColormapNotify, xcolormap)
  // BIND(ClientMessage, xclient)
  // BIND(MappingNotify, xmapping)
#undef BIND

#ifdef BUILD_XDAMAGE
  if (ev.type == window.xdamage_event_base + XDamageNotify) {
    XDamageNotifyEvent event = reinterpret_cast<XDamageNotifyEvent&>(ev);
    inner = event;
    return;
  }
#endif /* BUILD_XDAMAGE */

  bool has_cookie = ev.type == GenericEvent;

  if (has_cookie && ev.xgeneric.extension == window.xi_opcode) {
    if (!XGetEventData(display, &ev.xcookie)) {
      inner = event_error::ALREADY_CONSUMED;
      return;
    }

#define REIFY(name, type)                                           \
  case name: {                                                      \
    type event;                                                     \
    bool status = event.read_from_cookie(display, ev.xcookie.data); \
    if (status) {                                                   \
      inner = std::move(event);                                     \
    } else {                                                        \
      inner = event_error::PARSING_FAILURE;                         \
    }                                                               \
    XFreeEventData(display, &ev.xcookie);                           \
    return;                                                         \
  }

    switch (ev.xcookie.evtype) {
      REIFY(XI_ButtonPress, xi_pointer_press)
      REIFY(XI_ButtonRelease, xi_pointer_release)
      REIFY(XI_Motion, xi_pointer_move)
      REIFY(XI_Enter, xi_pointer_enter)
      REIFY(XI_Leave, xi_pointer_leave)
      REIFY(XI_FocusIn, xi_pointer_focus_in)
      REIFY(XI_FocusOut, xi_pointer_focus_out)
    }
#undef REIFY

    XFreeEventData(display, &ev.xcookie);
    inner = event_error::NOT_IMPLEMENTED;
    return;
  }

  inner = event_error::NOT_IMPLEMENTED;
}

int event::raw_x11_type() const {
  const int ResolveDynamically = -1;
  static std::array<int, std::variant_size_v<event::variant>> LOOKUP = ([]() {
    std::array<int, std::variant_size_v<event::variant>> result = {0};

    result[event_variant_index_of_v<event_error>] = 0;

    result[event_variant_index_of_v<XKeyEvent>] = ResolveDynamically;
    result[event_variant_index_of_v<XButtonEvent>] = ResolveDynamically;
    result[event_variant_index_of_v<XMotionEvent>] = MotionNotify;
    result[event_variant_index_of_v<XCrossingEvent>] = ResolveDynamically;
    result[event_variant_index_of_v<XFocusChangeEvent>] = ResolveDynamically;
    result[event_variant_index_of_v<XKeymapEvent>] = KeymapNotify;

    result[event_variant_index_of_v<XExposeEvent>] = Expose;
    result[event_variant_index_of_v<XGraphicsExposeEvent>] = GraphicsExpose;
    result[event_variant_index_of_v<XNoExposeEvent>] = NoExpose;
    result[event_variant_index_of_v<XVisibilityEvent>] = VisibilityNotify;
    result[event_variant_index_of_v<XCreateWindowEvent>] = CreateNotify;
    result[event_variant_index_of_v<XDestroyWindowEvent>] = DestroyNotify;
    result[event_variant_index_of_v<XUnmapEvent>] = UnmapNotify;
    result[event_variant_index_of_v<XMapEvent>] = MapNotify;
    result[event_variant_index_of_v<XMapRequestEvent>] = MapRequest;
    result[event_variant_index_of_v<XReparentEvent>] = ReparentNotify;
    result[event_variant_index_of_v<XConfigureEvent>] = ConfigureNotify;
    result[event_variant_index_of_v<XConfigureRequestEvent>] = ConfigureRequest;
    result[event_variant_index_of_v<XGravityEvent>] = GravityNotify;
    result[event_variant_index_of_v<XResizeRequestEvent>] = ResizeRequest;
    result[event_variant_index_of_v<XCirculateEvent>] = CirculateNotify;
    result[event_variant_index_of_v<XCirculateRequestEvent>] = CirculateRequest;
    result[event_variant_index_of_v<XPropertyEvent>] = PropertyNotify;
    result[event_variant_index_of_v<XSelectionClearEvent>] = SelectionClear;
    result[event_variant_index_of_v<XSelectionRequestEvent>] = SelectionRequest;
    result[event_variant_index_of_v<XSelectionEvent>] = SelectionNotify;
    result[event_variant_index_of_v<XColormapEvent>] = ColormapNotify;
    result[event_variant_index_of_v<XClientMessageEvent>] = ClientMessage;
    result[event_variant_index_of_v<XMappingEvent>] = MappingNotify;

#ifdef BUILD_XDAMAGE
    result[event_variant_index_of_v<XDamageNotifyEvent>] = ResolveDynamically;
#endif /* BUILD_XDAMAGE */

    result[event_variant_index_of_v<xi_pointer_move>] = GenericEvent;
    result[event_variant_index_of_v<xi_pointer_press>] = GenericEvent;
    result[event_variant_index_of_v<xi_pointer_release>] = GenericEvent;
    result[event_variant_index_of_v<xi_pointer_enter>] = GenericEvent;
    result[event_variant_index_of_v<xi_pointer_leave>] = GenericEvent;
    result[event_variant_index_of_v<xi_pointer_focus_in>] = GenericEvent;
    result[event_variant_index_of_v<xi_pointer_focus_out>] = GenericEvent;

    return result;
  })();

  auto value = LOOKUP[variant_index()];
  if (value != ResolveDynamically) { return value; }

#ifdef BUILD_XDAMAGE
  if (std::holds_alternative<XDamageNotifyEvent>(inner)) {
    return window.xdamage_event_base + XDamageNotify;
  }
#endif /* BUILD_XDAMAGE */

  // These are here for completeness. They will likely never be reached
  if (std::holds_alternative<XKeyEvent>(inner)) {
    return std::get<XKeyEvent>(inner).type;
  }
  if (std::holds_alternative<XButtonEvent>(inner)) {
    return std::get<XButtonEvent>(inner).type;
  }
  if (std::holds_alternative<XCrossingEvent>(inner)) {
    return std::get<XCrossingEvent>(inner).type;
  }
  if (std::holds_alternative<XFocusChangeEvent>(inner)) {
    return std::get<XFocusChangeEvent>(inner).type;
  }

  return 0;
}

#define DIRECT_DOWNCAST(value)                                     \
  template <>                                                      \
  std::optional<std::reference_wrapper<value>> event::downcast() { \
    if (std::holds_alternative<value>(inner)) {                    \
      return std::optional<std::reference_wrapper<value>>(         \
          std::reference_wrapper(std::get<value>(inner)));         \
    }                                                              \
    return std::nullopt;                                           \
  }

DIRECT_DOWNCAST(XKeyEvent)
DIRECT_DOWNCAST(XButtonEvent)
DIRECT_DOWNCAST(XMotionEvent)
DIRECT_DOWNCAST(XCrossingEvent)
DIRECT_DOWNCAST(XFocusChangeEvent)
DIRECT_DOWNCAST(XKeymapEvent)
DIRECT_DOWNCAST(XExposeEvent)
DIRECT_DOWNCAST(XGraphicsExposeEvent)
DIRECT_DOWNCAST(XNoExposeEvent)
DIRECT_DOWNCAST(XVisibilityEvent)
DIRECT_DOWNCAST(XCreateWindowEvent)
DIRECT_DOWNCAST(XDestroyWindowEvent)
DIRECT_DOWNCAST(XUnmapEvent)
DIRECT_DOWNCAST(XMapEvent)
DIRECT_DOWNCAST(XMapRequestEvent)
DIRECT_DOWNCAST(XReparentEvent)
DIRECT_DOWNCAST(XConfigureEvent)
DIRECT_DOWNCAST(XConfigureRequestEvent)
DIRECT_DOWNCAST(XGravityEvent)
DIRECT_DOWNCAST(XResizeRequestEvent)
DIRECT_DOWNCAST(XCirculateEvent)
DIRECT_DOWNCAST(XCirculateRequestEvent)
DIRECT_DOWNCAST(XPropertyEvent)
DIRECT_DOWNCAST(XSelectionClearEvent)
DIRECT_DOWNCAST(XSelectionRequestEvent)
DIRECT_DOWNCAST(XSelectionEvent)
DIRECT_DOWNCAST(XColormapEvent)
DIRECT_DOWNCAST(XClientMessageEvent)
DIRECT_DOWNCAST(XMappingEvent)
#ifdef BUILD_XDAMAGE
DIRECT_DOWNCAST(XDamageNotifyEvent)
#endif
DIRECT_DOWNCAST(xi_pointer_move)
DIRECT_DOWNCAST(xi_pointer_press)
DIRECT_DOWNCAST(xi_pointer_release)
DIRECT_DOWNCAST(xi_pointer_enter)
DIRECT_DOWNCAST(xi_pointer_leave)
DIRECT_DOWNCAST(xi_pointer_focus_in)
DIRECT_DOWNCAST(xi_pointer_focus_out)
#undef DIRECT_DOWNCAST

#define INDIRECT_CAST(from, to)                                           \
  if (std::holds_alternative<from>(inner)) {                              \
    return std::optional<std::reference_wrapper<to>>(                     \
        std::reference_wrapper(static_cast<to&>(std::get<from>(inner)))); \
  }

template <>
std::optional<std::reference_wrapper<xi_pointer_interact_event>>
event::downcast() {
  INDIRECT_CAST(xi_pointer_move, xi_pointer_interact_event)
  INDIRECT_CAST(xi_pointer_press, xi_pointer_interact_event)
  INDIRECT_CAST(xi_pointer_release, xi_pointer_interact_event)
  return std::nullopt;
}

template <>
std::optional<std::reference_wrapper<xi_pointer_crossing_event>>
event::downcast() {
  INDIRECT_CAST(xi_pointer_enter, xi_pointer_crossing_event)
  INDIRECT_CAST(xi_pointer_leave, xi_pointer_crossing_event)
  INDIRECT_CAST(xi_pointer_focus_in, xi_pointer_crossing_event)
  INDIRECT_CAST(xi_pointer_focus_out, xi_pointer_crossing_event)
  return std::nullopt;
}

template <>
std::optional<std::reference_wrapper<xi_pointer_event>> event::downcast() {
  INDIRECT_CAST(xi_pointer_move, xi_pointer_event)
  INDIRECT_CAST(xi_pointer_press, xi_pointer_event)
  INDIRECT_CAST(xi_pointer_release, xi_pointer_event)
  INDIRECT_CAST(xi_pointer_enter, xi_pointer_event)
  INDIRECT_CAST(xi_pointer_leave, xi_pointer_event)
  INDIRECT_CAST(xi_pointer_focus_in, xi_pointer_event)
  INDIRECT_CAST(xi_pointer_focus_out, xi_pointer_event)
  return std::nullopt;
}

template <>
std::optional<std::reference_wrapper<xi_event_base>> event::downcast() {
  INDIRECT_CAST(xi_pointer_move, xi_event_base)
  INDIRECT_CAST(xi_pointer_press, xi_event_base)
  INDIRECT_CAST(xi_pointer_release, xi_event_base)
  INDIRECT_CAST(xi_pointer_enter, xi_event_base)
  INDIRECT_CAST(xi_pointer_leave, xi_event_base)
  INDIRECT_CAST(xi_pointer_focus_in, xi_event_base)
  INDIRECT_CAST(xi_pointer_focus_out, xi_event_base)
  return std::nullopt;
}

#undef INDIRECT_CAST

template <bool Traverse>
inline bool check_window(const event& on, Window window) {
  // GenericEvent intentionally excluded - if not explicitly handled, we can't
  // tell where the window field is.
  if (on.raw_x11_type() <= MappingNotify) {
// Pointer/crossing events name a one-hop spatial child in `subwindow`: the
// listener is always `window`, the subject is that child when present.
#define POINTS_TO_SUBWINDOW(event, event_ty) \
  case event: {                              \
    auto& ev = std::get<event_ty>(on.inner); \
    if (Traverse && ev.subwindow != None) {  \
      return window == ev.subwindow;         \
    } else {                                 \
      return window == ev.window;            \
    }                                        \
  }
// Every other core event states both axes explicitly: `listener_field` is the
// window the event was delivered to, `subject_field` is the window it's about.
// Where the two coincide, the same field is passed twice.
#define LISTENER_SUBJECT(event, event_ty, listener_field, subject_field)    \
  case event:                                                               \
    return window == (Traverse ? std::get<event_ty>(on.inner).subject_field \
                               : std::get<event_ty>(on.inner).listener_field);

    switch (on.raw_x11_type()) {  // clang-format off
      POINTS_TO_SUBWINDOW(KeyPress, XKeyEvent)
      POINTS_TO_SUBWINDOW(KeyRelease, XKeyEvent)
      POINTS_TO_SUBWINDOW(ButtonPress, XButtonEvent)
      POINTS_TO_SUBWINDOW(ButtonRelease, XButtonEvent)
      POINTS_TO_SUBWINDOW(MotionNotify, XMotionEvent)
      POINTS_TO_SUBWINDOW(EnterNotify, XCrossingEvent)
      POINTS_TO_SUBWINDOW(LeaveNotify, XCrossingEvent)
      LISTENER_SUBJECT(FocusIn, XFocusChangeEvent, window, window)
      LISTENER_SUBJECT(FocusOut, XFocusChangeEvent, window, window)
      LISTENER_SUBJECT(KeymapNotify, XKeymapEvent, window, window)
      LISTENER_SUBJECT(Expose, XExposeEvent, window, window)
      case GraphicsExpose:
      case NoExpose:
        // Has only drawable
        return false;
      LISTENER_SUBJECT(VisibilityNotify, XVisibilityEvent, window, window)
      LISTENER_SUBJECT(CreateNotify, XCreateWindowEvent, parent, window)
      LISTENER_SUBJECT(DestroyNotify, XDestroyWindowEvent, event, window)
      LISTENER_SUBJECT(UnmapNotify, XUnmapEvent, event, window)
      LISTENER_SUBJECT(MapNotify, XMapEvent, event, window)
      LISTENER_SUBJECT(MapRequest, XMapRequestEvent, parent, window)
      LISTENER_SUBJECT(ReparentNotify, XReparentEvent, event, window)
      LISTENER_SUBJECT(ConfigureNotify, XConfigureEvent, event, window)
      LISTENER_SUBJECT(ConfigureRequest, XConfigureRequestEvent, parent, window)
      LISTENER_SUBJECT(GravityNotify, XGravityEvent, event, window)
      LISTENER_SUBJECT(ResizeRequest, XResizeRequestEvent, window, window)
      LISTENER_SUBJECT(CirculateNotify, XCirculateEvent, event, window)
      LISTENER_SUBJECT(CirculateRequest, XCirculateRequestEvent, parent, window)
      LISTENER_SUBJECT(PropertyNotify, XPropertyEvent, window, window)
      LISTENER_SUBJECT(SelectionClear, XSelectionClearEvent, window, window)
      LISTENER_SUBJECT(SelectionRequest, XSelectionRequestEvent, owner, requestor)
      LISTENER_SUBJECT(SelectionNotify, XSelectionEvent, requestor, requestor)
      LISTENER_SUBJECT(ColormapNotify, XColormapEvent, window, window)
      LISTENER_SUBJECT(ClientMessage, XClientMessageEvent, window, window)
      LISTENER_SUBJECT(MappingNotify, XMappingEvent, window, window)
    }  // clang-format on

#undef LISTENER_SUBJECT
#undef POINTS_TO_SUBWINDOW
  }

#ifdef BUILD_XDAMAGE
  if (auto xdamage_event = on.downcast<XDamageNotifyEvent>()) {
    // While a drawable (pixmap) can belong to a window, it would be incorrect
    // to handle that check here.
    return false;
  }
#endif /* BUILD_XDAMAGE */
  if (auto xi_event = on.downcast<xi_pointer_event>()) {
    auto& event = xi_event->get();
    if (Traverse && event.child != None) {
      // Not exhaustive enough for deep hierarchies, but it's highly unlikely
      // we'll introduce three or more levels of window hierarchy to conky.
      return xi_event->get().child == window;
    } else {
      return xi_event->get().event == window;
    }
  }
  return false;
}

bool event::is_window_listener(Window window) const {
  return check_window<false>(*this, window);
}

bool event::is_window_subject(Window window) const {
  return check_window<true>(*this, window);
}

}  // namespace conky::x11
