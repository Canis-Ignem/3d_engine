#include <vector>
#include "skybox.h"
#include "tools.h"
#include "vector3.h"
#include "trfm3D.h"
#include "renderState.h"
#include "gObjectManager.h"
#include "nodeManager.h"
#include "textureManager.h"
#include "materialManager.h"
#include "shaderManager.h"


using std::vector;
using std::string;

static const char *cmap_faces[6] = { "xpos", "xneg", "ypos", "yneg", "zpos", "zneg" };

static vector<string> generate_names(const string & dirname) {

	static const string ext(".jpg");
	vector<string> names;
	for(size_t i = 0; i < 6; i++) {
		string fname = getFilename(dirname, string(cmap_faces[i]));
		fname.append(".jpg");
		names.push_back(fname);
	}
	return names;
}

// TODO: create skybox object given gobject, shader and dir where textures are.
void CreateSkybox(GObject *gobj,
				  ShaderProgram *skyshader,
				  const std::string &dirname) {
}

// TODO: display the skybox
void DisplaySky(Camera *cam) {

	RenderState *rs = RenderState::instance();

	Node *skynode = rs->getSkybox();
	if (!skynode) return;
}
