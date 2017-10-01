#pragma once

#include <glibmm/ustring.h>
#include <memory>
#include "ai/common/player.h"
#include "drive/robot.h"

namespace AI
{
namespace BE
{
namespace Primitives
{
class PrimitiveDescriptor
{
   public:
    /**
     * \breif Default ctor.
     */
    PrimitiveDescriptor();

    /**
     * \brief Constructs a primitive descriptor with the given arguments.  */
    PrimitiveDescriptor(
        Drive::Primitive prim, double p0, double p1, double p2, double p3,
        uint8_t extra);

    /**
     * \brief Constructs a primitive descriptor with the given arguments.
     */
    PrimitiveDescriptor(
        Drive::Primitive prim, const std::array<double, 4>& arr, uint8_t extra);

    Point field_point() const;
    Angle field_angle() const;
    Angle field_angle_2() const;

    Drive::Primitive prim;
    std::array<double, 4> params;
    uint8_t extra;
};

inline Point PrimitiveDescriptor::field_point() const
{
    return Point(params[0], params[1]);
}

inline Angle PrimitiveDescriptor::field_angle() const
{
    return Angle::of_radians(params[2]);
}

inline Angle PrimitiveDescriptor::field_angle_2() const
{
    return Angle::of_radians(params[3]);
}

/**
 * \brief A primitive is an object that "abuses" C++'s deterministic destruction
 * to place itself on the robot's execution queue. When created, it is added to
 * the queue,
 * and when deleted, it is removed. If a primitive is destroyed when it isn't
 * the element
 * in the back of the queue, an exception is thrown.
 */
class Primitive : public NonCopyable
{
   public:
    using Ptr = std::unique_ptr<Primitive>;

    Primitive() = delete;

    /**
     * \brief Constructs a primitive with the given arguments.
     */
    Primitive(
        AI::Common::Player player, Drive::Primitive prim, double p0, double p1,
        double p2, double p3, uint8_t extra);

    /**
     * \brief Constructs a primitive with the given arguments.
     */
    Primitive(AI::Common::Player, PrimitiveDescriptor desc);

    /**
     * \brief Move ctor.
     */
    Primitive(Primitive&& other);

    /**
     * \brief Destroys the Primitive, removing it from the queue.
     */
    virtual ~Primitive();

    /**
     * \brief Checks if the primitive is done.
     *
     * The Navigator must set this flag.
     */
    bool done() const;
    void done(bool value);

    /**
     * \brief Checks if the primitive is being processed.
     *
     * The Navigator must set this flag.
     */
    bool active() const;
    void active(bool value);

    const PrimitiveDescriptor& desc() const;

    /**
     * \brief Gets the error thrown by this primitive, if it failed.
     * If this primitive has not failed, error() returns a nullptr.
     */
    std::exception_ptr error() const;

   private:
    const PrimitiveDescriptor desc_;
    std::unique_ptr<std::exception> error_;
    AI::Common::Player player_;
    bool done_;
    bool active_;
    bool moved_;
};

inline bool Primitive::done() const
{
    return done_;
}

inline void Primitive::done(bool value)
{
    done_ = value;
}

inline bool Primitive::active() const
{
    return active_;
}

inline void Primitive::active(bool value)
{
    active_ = value;
}

inline std::exception_ptr Primitive::error() const
{
    return std::make_exception_ptr(*error_);
}

inline const PrimitiveDescriptor& Primitive::desc() const
{
    return desc_;
}
}
}
}
