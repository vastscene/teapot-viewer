#include <stdio.h>

#include <g3d/context.h>
#include <g3d/model.h>

#include <SceneIO.h>

extern "C"
#if defined(_MSC_VER)
__declspec(dllexport)
#endif
eh::SceneIO::IPlugIn* XcreatePlugIn();

int main()
{
	XcreatePlugIn();

	printf("test");

	G3DContext* context = g3d_context_new();
	if( G3DModel *model = g3d_model_load_full(context, "test.vrml", 0) )
	{
		for(GSList *oitem = model->objects; oitem != NULL; oitem = oitem->next)
		{
			G3DObject *object = (G3DObject *)oitem->data;
			
			for(GSList *fitem = object->faces; fitem != NULL; fitem = fitem->next)
			{
				G3DFace *face = (G3DFace *)fitem->data;

				printf("face %d: vertices: %d\n", face, face->vertex_count);
			}
		}

		g3d_model_free(model);
	}
	g3d_context_free(context);
}