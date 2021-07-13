#include <cstdio>
#include <cassert>
#include "node.h"
#include "nodeManager.h"
#include "intersect.h"
#include "bboxGL.h"
#include "renderState.h"

using std::string;
using std::list;
using std::map;

// Recipe 1: iterate through children:
//
//    for(list<Node *>::iterator it = m_children.begin(), end = m_children.end();
//        it != end; ++it) {
//        Node *theChild = *it;
//        theChild->print(); // or any other thing
//    }

// Recipe 2: iterate through children inside a const method
//
//    for(list<Node *>::const_iterator it = m_children.begin(), end = m_children.end();
//        it != end; ++it) {
//        const Node *theChild = *it;
//        theChild->print(); // or any other thing
//    }

Node::Node(const string &name) :
	m_name(name),
	m_parent(0),
	m_gObject(0),
	m_light(0),
	m_shader(0),
	m_placement(new Trfm3D),
	m_placementWC(new Trfm3D),
	m_containerWC(new BBox),
	m_checkCollision(true),
	m_isCulled(false),
	m_drawBBox(false) {}

Node::~Node() {
	delete m_placement;
	delete m_placementWC;
	delete m_containerWC;
}

static string name_clone(const string & base) {

	static char MG_SC_BUFF[2048];
	Node *aux;

	int i;
	for(i = 1; i < 1000; i++) {
		sprintf(MG_SC_BUFF, "%s#%d", base.c_str(), i);
		string newname(MG_SC_BUFF);
		aux = NodeManager::instance()->find(newname);
		if(!aux) return newname;
	}
	fprintf(stderr, "[E] Node: too many clones of %s\n.", base.c_str());
	exit(1);
	return string();
}

Node* Node::cloneParent(Node *theParent) {

	string newName = name_clone(m_name);
	Node *newNode = NodeManager::instance()->create(newName);
	newNode->m_gObject = m_gObject;
	newNode->m_light = m_light;
	newNode->m_shader = m_shader;
	newNode->m_placement->clone(m_placement);
	newNode->m_parent = theParent;
	for(list<Node *>::iterator it = m_children.begin(), end = m_children.end();
		it != end; ++it) {
		Node *theChild = *it;
		newNode->m_children.push_back(theChild->cloneParent(this));
	}
	return newNode;
}


Node *Node::clone() {
	return cloneParent(0);
}

///////////////////////////////////
// transformations

void Node::attachGobject(GObject *gobj ) {
	if (!gobj) {
		fprintf(stderr, "[E] attachGobject: no gObject for node %s\n", m_name.c_str());
		exit(1);
	}
	if (m_children.size()) {
		fprintf(stderr, "EW] Node::attachGobject: can not attach a gObject to node (%s), which already has children.\n", m_name.c_str());
		exit(1);
	}
	m_gObject = gobj;
	propagateBBRoot();
}

GObject *Node::detachGobject() {
	GObject *res = m_gObject;
	m_gObject = 0;
	return res;
}

GObject *Node::getGobject() {
	return m_gObject;
}

void Node::attachLight(Light *theLight) {
	if (!theLight) {
		fprintf(stderr, "[E] attachLight: no light for node %s\n", m_name.c_str());
		exit(1);
	}
	m_light = theLight;
}

Light * Node::detachLight() {
	Light *res = m_light;
	m_light = 0;
	return res;
}

void Node::attachShader(ShaderProgram *theShader) {
	if (!theShader) {
		fprintf(stderr, "[E] attachShader: empty shader for node %s\n", m_name.c_str());
		exit(1);
	}
	m_shader = theShader;
}

ShaderProgram *Node::detachShader() {
	ShaderProgram *theShader = m_shader;
	m_shader = 0;
	return theShader;
}

ShaderProgram *Node::getShader() {
	return m_shader;
}

void Node::setDrawBBox(bool b) { m_drawBBox = b; }
bool Node::getDrawBBox() const { return m_drawBBox; }

void Node::setCheckCollision(bool b) { m_checkCollision = b; }
bool Node::getCheckCollision() const { return m_checkCollision; }

///////////////////////////////////
// transformations

void Node::initTrfm() {
	Trfm3D I;
	m_placement->swap(I);
	// Update Geometric state
	updateGS();
}

void Node::setTrfm(const Trfm3D * M) {
	if (!M) {
		fprintf(stderr, "[E] setTrfm: no trfm for node %s\n", m_name.c_str());
		exit(1);
	}
	m_placement->clone(M);
	// Update Geometric state
	updateGS();
}

void Node::addTrfm(const Trfm3D * M) {
	if (!M) {
		fprintf(stderr, "[E] addTrfm: no trfm for node %s\n", m_name.c_str());
		exit(1);
	}
	m_placement->add(M);
	// Update Geometric state
	updateGS();
}

void Node::translate(const Vector3 & P) {
	static Trfm3D localT;
	localT.setTrans(P);
	addTrfm(&localT);
};

void Node::rotateX(float angle ) {
	static Trfm3D localT;
	localT.setRotX(angle);
	addTrfm(&localT);
};

void Node::rotateY(float angle ) {
	static Trfm3D localT;
	localT.setRotY(angle);
	addTrfm(&localT);
};

void Node::rotateZ(float angle ) {
	static Trfm3D localT;
	localT.setRotZ(angle);
	addTrfm(&localT);
};

void Node::scale(float factor ) {
	static Trfm3D localT;
	localT.setScale(factor);
	addTrfm(&localT);
};

///////////////////////////////////
// tree operations

Node *Node::parent() {
	if (!m_parent) return this;
	return m_parent;
}

/**
 * nextSibling: Get next sibling of node. Return first sibling if last child.
 *
 */

Node *Node::nextSibling() {
	Node *p = m_parent;
	if(!p) return this;
	list<Node *>::iterator end = p->m_children.end();
	list<Node *>::iterator it = p->m_children.begin();
	while(it != end && *it != this) ++it;
	assert(it != end);
	++it;
	if (it == end) return *(p->m_children.begin());
	return *it;
}

/**
 * firstChild: Get first child of node. Return itself if leaf node.
 *
 * @param theNode A pointer to the node
 */

Node *Node::firstChild() {
	if (!m_children.size()) return this;
	return *(m_children.begin());
}

// Cycle through children of a Node. Return children[ i % lenght(children) ]

Node * Node::cycleChild(size_t idx) {

	size_t m = idx % m_children.size();
	size_t i = 0;
	for(list<Node *>::iterator it = m_children.begin(), end = m_children.end();
		it != end; ++it) {
		if (i == m) return *it;
		i++;
	}
	return 0;
}

// @@ TODO:
//
// Add a child to node
// Print a warning (and do nothing) if node already has an gObject.

void Node::addChild(Node *theChild) {

	printf("addChild from node");
	if (theChild == 0 || this == 0){
		
	 	return;
	}

	if (m_gObject!=0) {
		//Si el nodo tiene un objeto no se le puede añadir un hijo es una HOJA
		printf("this node already has a gObject, carefull!!");
	} else {
		//Si no tiene es una hoja se añade y actualiza
		this->m_children.push_back(theChild);
		theChild->m_parent = this;
		theChild->updateGS();
	}
	
}

void Node::detach() {

	Node *theParent;
	theParent = m_parent;
	if (theParent == 0) return; // already detached (or root node)
	m_parent = 0;
	theParent->m_children.remove(this);
	// Update bounding box of parent
	theParent->propagateBBRoot();
}

// @@ TODO: auxiliary function
//
// given a node:
// - update the BBox of the node (updateBB)
// - Propagate BBox to parent until root is reached
//
// - Preconditions:
//    - the BBox of thisNode's children are up-to-date.
//    - placementWC of node and parents are up-to-date

void Node::propagateBBRoot() {
	printf("propagating BB to root\n");
	
	//Actualziamos su BB y llamamos hasta llegar al padre
	this->updateBB();
	if(this->m_parent != 0){
		this->m_parent->propagateBBRoot();
	}
}

// @@ TODO: auxiliary function
//
// given a node, update its BBox to World Coordinates so that it includes:
//  - the BBox of the geometricObject it contains (if any)
//  - the BBox-es of the children
//
// Note: Bounding box is always in world coordinates
//
// Precontitions:
//
//     * m_placementWC is up-to-date
//     * m_containerWC of 'this' children are up-to-date
//
// These functions can be useful
//
// Member functions of BBox class (from Math/bbox.h):
//
//    void init(): Set BBox to be the void (empty) BBox
//    void clone(const BBox * source): Copy from source bounding box
//    void transform(const Trfm3D * T): Transform BBox by applying transformation T
//    void include(BBox *B): Change BBox so that it also includes BBox B
//
// Note:
//    See Recipe 1 in for knowing how to iterate through children.

void Node::updateBB () {

	printf("updateBB\n");
	
	// iniciamos la BB del objeto
	this->m_containerWC->init();
	//SI es un nodo oja se clona la BB de su objeto y se le aplica la transformacion global
	if(this->m_gObject != 0){
		
		this->m_containerWC->clone(m_gObject->getContainer());
		this->m_containerWC->transform(m_placementWC);

	}else{
		
		for(list<Node *>::iterator it = m_children.begin(), end = m_children.end(); it != end; ++it) {

        	Node *theChild = *it;
			//Si no es hoja se incluyen todas las BB de sus nodos hijo
			
			this->m_containerWC->include(theChild->m_containerWC);
			
			
			
	    }

	}

}

// @@ TODO: Update WC (world coordinates matrix) of a node and
// its bounding box recursively updating all children.
//
// given a node:
//  - update the world transformation (m_placementWC) using the parent's WC transformation
//  - recursive call to update WC of the children
//  - update Bounding Box to world coordinates
//
// Precondition:
//
//  - m_placementWC of m_parent is up-to-date (or m_parent == 0)
//
// Note:
//    See Recipe 1 in for knowing how to iterate through children.

void Node::updateWC() {
	printf("updateWC\n");
	
	
	
	if(this->m_parent == 0){
		//SI es el nodo raiz sus transformacion global es la suyo local
		this->m_containerWC->init();
		this->m_placementWC->clone(m_placement);
		
	}else{
		//SI es un nodo hijo se la mezcla de la transformacion global del padre seguida de la suya
		Trfm3D auxT;
		auxT.clone(this->m_parent->m_placementWC);
		auxT.add(this->m_placement);
		this->m_placementWC->clone(auxT);

	}
	//Actualizar todos los hijos y la BB
	for(list<Node *>::iterator it = m_children.begin(), end = m_children.end();it != end; ++it) {
        Node *theChild = *it;
        theChild->updateWC();
    }

	updateBB();

	
}

// @@ TODO:
//
//  Update geometric state of a node.
//
// given a node:
// - Update WC transformation of sub-tree starting at node (updateWC)
// - Propagate Bounding Box to root (propagateBBRoot), starting from the parent, if parent exists.

void Node::updateGS() {

	
	//Primero actualizar las transformaciones luego propagar hacia arriba las BBs
	updateWC();
	if(this->m_parent != 0){
		this->m_parent->propagateBBRoot();
	}
}

// @@ TODO:
// Draw a (sub)tree.
//
// These functions can be useful:
//
// RenderState *rs = RenderState::instance();
//
// rs->addTrfm(RenderState::modelview, T); // Add T transformation to modelview
// rs->push(RenderState::modelview); // push current matrix into modelview stack
// rs->pop(RenderState::modelview); // pop matrix from modelview stack to current
//
// gobj->draw(); // draw geometry object (gobj)
//
// Note:
//    See Recipe 1 in for knowing how to iterate through children.

void Node::draw() {
	ShaderProgram *prev_shader = 0;
	RenderState *rs = RenderState::instance();

	if (m_isCulled) return;

	// Set shader (save previous)
	if (m_shader != 0) {
		prev_shader = rs->getShader();
		rs->setShader(m_shader);
	}

	// Print BBoxes
	if(rs->getBBoxDraw() || m_drawBBox)
		BBoxGL::draw( m_containerWC );

	/* =================== PUT YOUR CODE HERE ====================== */
	//Cargamos el modelview y metemos la transformacion
	

	//Si no tiene objeto entramos en el caso recursivo
	if(this->m_gObject == 0){ // caso recursivo
		printf("Caso recursivo del draw\n");
		//SI no tiene objeto buscamos por todos sus hijos hasta encontrar nodos hoja con
		for(list<Node *>::iterator it = m_children.begin(), end = m_children.end();
				it != end; ++it) {
				Node *theChild = *it;
				theChild->draw();				
			}

	}else{// caso basico de recursion estoy en un nodo hoja solo dibujar y acabar
			//SI es un nodo hoja cargamos la matriz modelview metemos la transformacion global del nodo dibujamos y quitamos la de transformacion
			printf("Caso basico del draw\n");
			rs->push(RenderState::modelview);
			rs->addTrfm(RenderState::modelview, this->m_placementWC);
			//rs->loadTrfm(RenderState::model, m_placementWC);
			this->m_gObject->draw();
			rs->pop(RenderState::modelview);
	}
	

	/* =================== END YOUR CODE HERE ====================== */

	// Restore shaders
	if (prev_shader != 0) {
		// restore shader
		rs->setShader(prev_shader);
	}
}

// Set culled state of a node's children

void Node::setCulled(bool culled) {
	m_isCulled = culled;
	for(list<Node *>::iterator it = m_children.begin(), end = m_children.end();
		it != end; ++it) {
		Node *theChild = *it;
		theChild->setCulled(culled); // Recursive call
	}
}

// @@ TODO: Frustum culling. See if a subtree is culled by the camera, and
//          update m_isCulled accordingly.

void Node::frustumCull(Camera *cam) {

		int checked = cam->checkFrustum(m_containerWC,0);

		if(checked == 1){
			setCulled(1);
		}else if(checked == 0)
		{
			m_isCulled = 0;
			for(list<Node *>::iterator it = m_children.begin(), end = m_children.end();
	        it != end; ++it) {

				Node *theChild = *it;
				theChild->frustumCull(cam);


			}
		}
		else
		{
			setCulled(0);
		}
}


// @@ TODO: Check whether a BSphere (in world coordinates) intersects with a
// (sub)tree.
//
// Return a pointer to the Node which collides with the BSphere. 0
// if not collision.
//
// Note:
//    See Recipe 2 in for knowing how to iterate through children inside a const
//    method.

const Node *Node::checkCollision(const BSphere *bsph) const {
	if (!m_checkCollision) return 0;
	/* =================== PUT YOUR CODE HERE ====================== */
	//El avatar es una esfera, si no colisiona X > 0 entonces nada
	if(BSphereBBoxIntersect(bsph,this->m_containerWC)){
		return 0;

		}else{
			//Si colisiona con la BB de este nodo, si es un nodo hoja entonces devuelves el nodo
			if (this->m_gObject){
				return this;
			} else {
				//Si no es nodo hoja entonces tenemos que buscar si choca con alguno de sus hijos hasta que choque con un nodo hoja
				const Node *auxN;
				for(list<Node *>::const_iterator it = m_children.begin(), end = m_children.end(); it != end; ++it) {
					Node *theChild = *it;
					auxN = theChild->checkCollision(bsph);
					if (auxN){
						return auxN;
					}
				}
				//Si no choca con un nodo hoja  entonces NO colisiona
				return 0;
		}
	}

	return 0; /* No collision */
	/* =================== END YOUR CODE HERE ====================== */
}
