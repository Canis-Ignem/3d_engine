#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "line.h"
#include "constants.h"
#include "tools.h"

Line::Line() : m_O(Vector3::ZERO), m_d(Vector3::UNIT_Y) {}
Line::Line(const Vector3 & o, const Vector3 & d) : m_O(o), m_d(d) {}
Line::Line(const Line & line) : m_O(line.m_O), m_d(line.m_d) {}

Line & Line::operator=(const Line & line) {
	if (&line != this) {
		m_O = line.m_O;
		m_d = line.m_d;
	}
	return *this;
}

// @@ TODO: Set line to pass through two points A and B
//
// Note: Check than A and B are not too close!

void Line::setFromAtoB(const Vector3 & A, const Vector3 & B) {
	printf("Line from A to B");
	Vector3 test;
	test = B-A;

	//comprobamos que la distancia no sea 0, si lo es no se podra trazar la linea
	if(	!test.isZero()) {
		
		m_O = A;
		m_d = test;
		m_d.normalize();

	}else
	{
		printf("Estos puntos se hayan demasiado cerca.");
	}
	

}

// @@ TODO: Give the point corresponding to parameter u

Vector3 Line::at(float u) const {
	printf("at the from line");
	Vector3 res;
	res = m_O + u*m_d;
	return res;
}

// @@ TODO: Calculate the parameter 'u0' of the line point nearest to P
//
// u0 = D*(P-O) / D*D , where * == dot product

float Line::paramDistance(const Vector3 & P) const {
	float res = 0.0f;
	printf("paramDistance from line");

	float dotDD = m_d.dot(m_d);
	Vector3 PO = P - m_O;
	float dotDPO = m_d.dot(PO);

	if(dotDD > Vector3::epsilon){
	res = dotDPO / dotDD;

	return res;
	}
	else {
	printf("Ha habido un error de calculo");
	return -1;
	}
}

// @@ TODO: Calculate the minimum distance 'dist' from line to P
//
// dist = ||P - (O + uD)||
// Where u = paramDistance(P)

float Line::distance(const Vector3 & P) const {
	float res = 0.0f;
	Vector3 vec;
	float u = paramDistance(P);
	if (u){
		vec = P-at(u);
		res = vec.length();
	} 
	else{
		vec = m_O-P;
		return vec.length();
	}
	return res;
}

void Line::print() const {
	printf("O:");
	m_O.print();
	printf(" d:");
	m_d.print();
	printf("\n");
}
