#pragma once

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/region.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/util.h"
#include "ai/hl/world.h"

using AI::Common::PlayType;
using namespace AI::HL::STP::Predicates;
using namespace AI::HL::STP;

using AI::HL::STP::TEAM_MAX_SIZE;
using AI::HL::STP::DIST_FROM_PENALTY_MARK;

#define BEGIN_DEC(cls)                                                         \
    namespace                                                                  \
    {                                                                          \
    class cls##Play;                                                           \
    class cls##PlayFactory;                                                    \
                                                                               \
    class cls##PlayFactory                                                     \
        : public AI::HL::STP::Play::PlayFactoryImpl<cls##Play>                 \
    {                                                                          \
       public:                                                                 \
        explicit cls##PlayFactory(const char *name)                            \
            : AI::HL::STP::Play::PlayFactoryImpl<cls##Play>(name)              \
        {                                                                      \
        }

#define INVARIANT(expr)                                                        \
    inline bool invariant(World world) const override                          \
    {                                                                          \
        return expr;                                                           \
    }

#define APPLICABLE(expr)                                                       \
    inline bool applicable(World world) const override                         \
    {                                                                          \
        return expr;                                                           \
    }

#define END_DEC(cls)                                                           \
    }                                                                          \
    ;                                                                          \
    }                                                                          \
    ::cls##PlayFactory cls##PlayFactory_instance(u8## #cls);

#define BEGIN_DEF(cls)                                                         \
    namespace                                                                  \
    {                                                                          \
    class cls##Play final : public AI::HL::STP::Play::Play                     \
    {                                                                          \
       public:                                                                 \
        explicit cls##Play(AI::HL::W::World world)                             \
            : AI::HL::STP::Play::Play(world)                                   \
        {                                                                      \
        }                                                                      \
                                                                               \
        AI::HL::STP::Play::PlayFactory &factory() const override               \
        {                                                                      \
            return cls##PlayFactory_instance;                                  \
        }

#define DONE(expr)                                                             \
    bool done() const override                                                 \
    {                                                                          \
        return expr;                                                           \
    }

#define FAIL(expr)                                                             \
    bool fail() const override                                                 \
    {                                                                          \
        return expr;                                                           \
    }

#define EXECUTE()                                                              \
    void execute(caller_t &caller) override                                    \
    {
#define END_DEF(cls)                                                           \
    }                                                                          \
    }                                                                          \
    ;                                                                          \
    }
