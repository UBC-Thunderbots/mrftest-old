#ifndef UTIL_JOYSTICK_H
#define UTIL_JOYSTICK_H

#include <glibmm/ustring.h>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/property.h"

/**
 * \brief Provides access to a joystick attached to the computer.
 */
class Joystick final : public NonCopyable, public sigc::trackable
{
   public:
    /**
     * \brief The type of identifying metadata associated with a joystick.
     */
    struct Identifier final
    {
        Glib::ustring name;
        uint16_t bus_type, vendor_id, product_id, version;
    };

    /**
     * \brief The index of the /dev/input/event* ndoe for this joystick.
     */
    const unsigned int index;

    /**
     * \brief Returns the number of joysticks attached to the computer.
     *
     * \return the number of joysticks.
     */
    static std::size_t count();

    /**
     * \brief Returns a joystick by index.
     *
     * \pre 0 ≤ \p index < count()
     *
     * \param[in] index the zero-based index of the joystick to retrieve.
     *
     * \return the joystick.
     */
    static const Joystick &get(std::size_t index);

    /**
     * \brief Returns the joystick’s identifying metadata.
     *
     * \return the identifier
     */
    const Identifier &identifier() const;

    /**
     * \brief Returns the joystick’s physical bus location.
     *
     * \return the location
     */
    const Glib::ustring &physical_location() const;

    /**
     * \brief Emitted whenever any axis or button changes value.
     */
    sigc::signal<void> &signal_changed() const;

    /**
     * \brief Returns the joystick's axes.
     *
     * \return the axes.
     */
    const std::vector<Property<double>> &axes() const;

    /**
     * \brief Returns the joystick's buttons.
     *
     * \return the buttons.
     */
    const std::vector<Property<bool>> &buttons() const;

   private:
    struct AxisMetadata final
    {
        int32_t minimum, maximum, flat;
    };

    FileDescriptor fd;
    Identifier identifier_;
    Glib::ustring physical_location_;
    mutable sigc::signal<void> signal_changed_;
    std::unordered_map<unsigned int, unsigned int> axes_map, buttons_map;
    std::vector<AxisMetadata> axes_metadata;
    std::vector<Property<double>> axes_;
    std::vector<Property<bool>> buttons_;
    bool flushing_dropped;
    static std::vector<std::unique_ptr<Joystick>> &instances();

    explicit Joystick(unsigned int index);
    void on_readable();
    void read_all_axes();
    void read_all_buttons();
    static double convert_axis(int32_t value, const AxisMetadata &md);
};

/**
 * \brief Compares two identifiers for equality.
 *
 * Two identifiers are considered equal if their ID numbers are equal, ignoring
 * their names.
 *
 * \param[in] x the first identifier to compare
 * \param[in] y the second identifier to compare
 * \retval true x has the same ID numbers as y
 * \retval false x has different ID numbers to y
 */
inline bool operator==(
    const Joystick::Identifier &x, const Joystick::Identifier &y)
{
    return x.bus_type == y.bus_type && x.vendor_id == y.vendor_id &&
           x.product_id == y.product_id && x.version == y.version;
}

/**
 * \brief Compares two identifiers for inequality.
 *
 * Two identifiers are considered equal if their ID numbers are equal, ignoring
 * their names.
 *
 * \param[in] x the first identifier to compare
 * \param[in] y the second identifier to compare
 * \retval true x has different ID numbers to y
 * \retval false x has the same ID numbers as y
 */
inline bool operator!=(
    const Joystick::Identifier &x, const Joystick::Identifier &y)
{
    return !(x == y);
}

/**
 * \brief Compares two identifiers for ordering.
 *
 * Two identifiers are ordered by their ID numbers.
 *
 * \param[in] x the first identifier to compare
 * \param[in] y the second identifier to compare
 * \retval true x orders before y
 * \retval false x does not order before y
 */
inline bool operator<(
    const Joystick::Identifier &x, const Joystick::Identifier &y)
{
    if (x.bus_type != y.bus_type)
    {
        return x.bus_type < y.bus_type;
    }
    else if (x.vendor_id != y.vendor_id)
    {
        return x.vendor_id < y.vendor_id;
    }
    else if (x.product_id != y.product_id)
    {
        return x.product_id < y.product_id;
    }
    else
    {
        return x.version < y.version;
    }
}

namespace std
{
/**
 * \brief Hashes a joystick identifier.
 */
template <>
struct hash<Joystick::Identifier> final
{
    std::size_t operator()(const Joystick::Identifier &i) const
    {
        std::hash<uint16_t> h;
        return h(i.bus_type) * 65537U + h(i.vendor_id) * 257U +
               h(i.product_id) * 17U + h(i.version);
    }
};
}

inline std::size_t Joystick::count()
{
    return instances().size();
}

inline const Joystick &Joystick::get(std::size_t index)
{
    return *instances()[index].get();
}

inline const Joystick::Identifier &Joystick::identifier() const
{
    return identifier_;
}

inline const Glib::ustring &Joystick::physical_location() const
{
    return physical_location_;
}

inline sigc::signal<void> &Joystick::signal_changed() const
{
    return signal_changed_;
}

inline const std::vector<Property<double>> &Joystick::axes() const
{
    return axes_;
}

inline const std::vector<Property<bool>> &Joystick::buttons() const
{
    return buttons_;
}

#endif
