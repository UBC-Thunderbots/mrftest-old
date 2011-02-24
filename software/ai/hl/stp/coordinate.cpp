#include "ai/hl/stp/coordinate.h"
#include "util/dprint.h"

using AI::HL::STP::Coordinate;
using namespace AI::HL::W;

namespace {
	class Fixed : public Coordinate::CoordinateData {
		public:
			Fixed(const Point &p) : pos(p) {
			}

		private:
			Point pos;
			Point evaluate() const;
	};

	class BallUp : public Coordinate::CoordinateData {
		public:
			BallUp(const Ball &b, const Point &p) : ball(b), pos(p) {
			}

		private:
			const Ball &ball;
			Point pos;
			Point evaluate() const;
	};

	template<typename Ptr> class RelativeRole : public Coordinate::CoordinateData {
		public:
			RelativeRole(const Ptr &p, const Point &off) : obj(p), offset(off) {
			}

		private:
			Ptr obj;
			Point offset;
			Point evaluate() const;
	};

	template<typename Ptr> class OffsetRole : public Coordinate::CoordinateData {
		public:
			OffsetRole(const Ptr &p, const Point &off) : obj(p), offset(off) {
			}

		private:
			Ptr obj;
			Point offset;
			Point evaluate() const;
	};

	class OffsetBall : public Coordinate::CoordinateData {
		public:
			OffsetBall(const Ball &ball, const Point &p) : ball(ball), pos(p) {
			}

		private:
			const Ball &ball;
			Point pos;
			Point evaluate() const;
	};

	Point Fixed::evaluate() const {
		return pos;
	}

	Point BallUp::evaluate() const {
		if (ball.position().y < 0) {
			return Point(pos.x, -pos.y);
		} else {
			return pos;
		}
	}

	template<typename Ptr> Point RelativeRole<Ptr>::evaluate() const {
		if (!obj->evaluate().is()) {
			LOG_ERROR("Role has no robot!");
			return Point(0, 0);
		}
		return obj->evaluate()->position() + offset.rotate(obj->evaluate()->orientation());
	}

	template<typename Ptr> Point OffsetRole<Ptr>::evaluate() const {
		if (!obj->evaluate().is()) {
			LOG_ERROR("Role has no robot!");
			return Point(0, 0);
		}
		return obj->evaluate()->position() + offset;
	}

	Point OffsetBall::evaluate() const {
		return ball.position() + pos;
	}
}

Point Coordinate::operator()() const {
	if (!data.is()) {
		LOG_ERROR("Invalid Point");
		return Point(0, 0);
	}
	return data->evaluate();
}

Coordinate::Coordinate() : data(new Fixed(Point())) {
}

Coordinate::Coordinate(const Point &pos) : data(new Fixed(pos)) {
}

Coordinate Coordinate::fixed(const Point &pos) {
	const RefPtr<const CoordinateData> data(new Fixed(pos));
	return Coordinate(data);
}

Coordinate Coordinate::ball_up(const AI::HL::W::Ball &ball, const Point &pos) {
	const RefPtr<const CoordinateData> data(new BallUp(ball, pos));
	return Coordinate(data);
}

Coordinate Coordinate::relative(const Enemy::Ptr enemy, const Point &off) {
	const RefPtr<const CoordinateData> data(new RelativeRole<Enemy::Ptr>(enemy, off));
	return Coordinate(data);
}

Coordinate Coordinate::relative(const Role::Ptr player, const Point &off) {
	const RefPtr<const CoordinateData> data(new RelativeRole<Role::Ptr>(player, off));
	return Coordinate(data);
}

Coordinate Coordinate::offset(const Ball &ball, const Point &off) {
	const RefPtr<const CoordinateData> data(new OffsetBall(ball, off));
	return Coordinate(data);
}

Coordinate Coordinate::offset(const Enemy::Ptr enemy, const Point &off) {
	const RefPtr<const CoordinateData> data(new OffsetRole<Enemy::Ptr>(enemy, off));
	return Coordinate(data);
}

Coordinate Coordinate::offset(const Role::Ptr player, const Point &off) {
	const RefPtr<const CoordinateData> data(new OffsetRole<Role::Ptr>(player, off));
	return Coordinate(data);
}

Coordinate::Coordinate(const RefPtr<const CoordinateData> data) : data(data) {
}

