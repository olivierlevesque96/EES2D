/* ---------------------------------------------------------------------
 *
 * Copyright (C) 2020 - by the EES2D  authors
 *
 * This file is part of EES2D.
 *
 *   EES2D is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   EES2D is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with EES2D.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------
 *
 * Authors: Amin Ouled-Mohamed & Ali Omais, Polytechnique Montreal, 2020-
 */
#pragma once

#include <cmath>
#include <iostream>


namespace ees2d::utils {

	template <class T >
  class Vector2;

  template <class T>
  std::ostream& operator << (std::ostream& ,const Vector2<T>& );

  template<class T>
	class Vector2 {
		// Class to deal with 2D vectors
		// Should be declared and defined in header file because its templated
public:
		T x, y;

		Vector2() : x(0), y(0) {}
		Vector2(T x, T y) : x(x), y(y) {}

		//Copy constructor
		Vector2(const Vector2 &v) : x(v.x), y(v.y) {}

		// Copy assignment operator
		Vector2 &operator=(const Vector2 &v) {
			x = v.x;
			y = v.y;
			return *this;
		}


		Vector2 operator+(Vector2 &v) {
			return Vector2(x + v.x, y + v.y);
		}

		Vector2 operator+(Vector2 &&v) {
			return Vector2(x + v.x, y + v.y);
		}

		bool operator==(const Vector2 &v) const {
			return this->x == v.x && this->y == v.y;
		}


		Vector2 operator-(Vector2 &v) {
			return Vector2(x - v.x, y - v.y);
		}

		static double dot(Vector2 v1, Vector2 v2) {
			return v1.x * v2.x + v1.y * v2.y;
		}
		static double cross(Vector2 v1, Vector2 v2) {
			return (v1.x * v2.y) - (v1.y * v2.x);
		}

		Vector2 operator+(double s) {
			return Vector2(x + s, y + s);
		}
		Vector2 operator-(double s) {
			return Vector2(x - s, y - s);
		}
		Vector2 operator*(double s) {
			return Vector2(x * s, y * s);
		}
		Vector2 operator/(double s) {
			return Vector2(x / s, y / s);
		}

		void set(T x, T y) {
			this->x = x;
			this->y = y;
		}

		void rotate(double deg) {
			double theta = deg / 180.0 * M_PI;
			double c = cos(theta);
			double s = sin(theta);
			double tx = x * c - y * s;
			double ty = x * s + y * c;
			x = tx;
			y = ty;
		}


		double length() const {
			return std::sqrt(x * x + y * y);
		}

		friend std::ostream& operator << <> (std::ostream &os, const Vector2<T>& v);
	};

	template <class T> std::ostream& operator<< (std::ostream &os, const ees2d::utils::Vector2<T>& v) {
    os << "{ " << v.x << " , " << v.y << " }";
    return os;
  }
	// Print overloading


}// Namespace ees2d::utils
