#include "Function/FbxImporter.h"

#include <list>

namespace FireEngine
{
	void CFbxImporter::Initialize()
	{
		m_fbx_manager = FbxManager::Create();
		if (!m_fbx_manager)
		{
			printf("Error: Unable to create FBX Manager!\n");
		}
		else
		{
			printf("Autodesk FBX SDK version %s\n", m_fbx_manager->GetVersion());
		}

		//Create an IOSettings object. This object holds all import/export settings.
		FbxIOSettings* ios = FbxIOSettings::Create(m_fbx_manager, IOSROOT);
		m_fbx_manager->SetIOSettings(ios);

		//Load plugins from the executable directory (optional)
		FbxString path = FbxGetApplicationDirectory();
		m_fbx_manager->LoadPluginsDirectory(path.Buffer());

        m_geometry_converter = new FbxGeometryConverter(m_fbx_manager);
	}

	CFbxImporter::CFbxImporter()
	{
		Initialize();
	}

	CFbxImporter::~CFbxImporter()
	{
		delete m_geometry_converter;
        m_geometry_converter = nullptr;
		if (m_fbx_manager)
		{
            m_fbx_manager->Destroy();
            m_fbx_manager = nullptr;
		}
	}

	bool CFbxImporter::LoadScene(const std::string& file_name)
    {
        int lFileMajor, lFileMinor, lFileRevision;
        int lSDKMajor, lSDKMinor, lSDKRevision;
        //int lFileFormat = -1;
        int lAnimStackCount;
        bool lStatus;
        char lPassword[1024];

        // Get the file version number generate by the FBX SDK.
        FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

        // Create an importer.
        FbxImporter* lImporter = FbxImporter::Create(m_fbx_manager, "");

        // Initialize the importer by providing a filename.
        const bool lImportStatus = lImporter->Initialize(file_name.c_str(), -1, m_fbx_manager->GetIOSettings());
        lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

        if (!lImportStatus)
        {
            FbxString error = lImporter->GetStatus().GetErrorString();
            FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
            FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

            if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
            {
                FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
                FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", file_name.c_str(), lFileMajor, lFileMinor, lFileRevision);
            }

            return false;
        }

        FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

        if (lImporter->IsFBX())
        {
            FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", file_name.c_str(), lFileMajor, lFileMinor, lFileRevision);

            // From this point, it is possible to access animation stack information without
            // the expense of loading the entire file.

            FBXSDK_printf("Animation Stack Information\n");

            lAnimStackCount = lImporter->GetAnimStackCount();

            FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
            FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
            FBXSDK_printf("\n");

            for (int i = 0; i < lAnimStackCount; i++)
            {
                FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

                FBXSDK_printf("    Animation Stack %d\n", i);
                FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
                FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

                // Change the value of the import name if the animation stack should be imported 
                // under a different name.
                FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

                // Set the value of the import state to false if the animation stack should be not
                // be imported. 
                FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
                FBXSDK_printf("\n");
            }

            // Set the import states. By default, the import states are always set to 
            // true. The code below shows how to change these states.
            m_fbx_manager->GetIOSettings()->SetBoolProp(IMP_FBX_MATERIAL, true);
            m_fbx_manager->GetIOSettings()->SetBoolProp(IMP_FBX_TEXTURE, true);
            m_fbx_manager->GetIOSettings()->SetBoolProp(IMP_FBX_LINK, true);
            m_fbx_manager->GetIOSettings()->SetBoolProp(IMP_FBX_SHAPE, true);
            m_fbx_manager->GetIOSettings()->SetBoolProp(IMP_FBX_GOBO, true);
            m_fbx_manager->GetIOSettings()->SetBoolProp(IMP_FBX_ANIMATION, true);
            m_fbx_manager->GetIOSettings()->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
        }

        // Import the scene.
        m_fbx_scene = FbxScene::Create(m_fbx_manager, "My Scene");
        lStatus = lImporter->Import(m_fbx_scene);
        if (lStatus == true)
        {
            // Check the scene integrity!
            FbxStatus status;
            FbxArray< FbxString*> details;
            FbxSceneCheckUtility sceneCheck(FbxCast<FbxScene>(m_fbx_scene), &status, &details);
            lStatus = sceneCheck.Validate(FbxSceneCheckUtility::eCkeckData);
            bool lNotify = (!lStatus && details.GetCount() > 0) || (lImporter->GetStatus().GetCode() != FbxStatus::eSuccess);
            if (lNotify)
            {
                FBXSDK_printf("\n");
                FBXSDK_printf("********************************************************************************\n");
                if (details.GetCount())
                {
                    FBXSDK_printf("Scene integrity verification failed with the following errors:\n");
                    for (int i = 0; i < details.GetCount(); i++)
                        FBXSDK_printf("   %s\n", details[i]->Buffer());

                    FbxArrayDelete<FbxString*>(details);
                }

                if (lImporter->GetStatus().GetCode() != FbxStatus::eSuccess)
                {
                    FBXSDK_printf("\n");
                    FBXSDK_printf("WARNING:\n");
                    FBXSDK_printf("   The importer was able to read the file but with errors.\n");
                    FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
                    FBXSDK_printf("   Last error message:'%s'\n", lImporter->GetStatus().GetErrorString());
                }
                FBXSDK_printf("********************************************************************************\n");
                FBXSDK_printf("\n");
            }
        }

        if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
        {
            FBXSDK_printf("Please enter password: ");

            lPassword[0] = '\0';

            FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
                scanf("%s", lPassword);
            FBXSDK_CRT_SECURE_NO_WARNING_END

                FbxString lString(lPassword);

            m_fbx_manager->GetIOSettings()->SetStringProp(IMP_FBX_PASSWORD, lString);
            m_fbx_manager->GetIOSettings()->SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

            lStatus = lImporter->Import(m_fbx_scene);

            if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
            {
                FBXSDK_printf("\nPassword is wrong, import aborted.\n");
            }
        }

        // Destroy the importer.
        lImporter->Destroy();

        return lStatus;
    }

	void CFbxImporter::LoadStaticMesh(CMesh* out_mesh)
	{
		if (!m_fbx_scene)
		{
			return;
		}

		FbxNode* root_node = m_fbx_scene->GetRootNode();
       
        std::vector<FbxMesh*> meshes;
		{
            std::vector<FbxNode*> node_stack;
            node_stack.emplace_back(root_node);
            uint64_t node_index = 0;
            while (node_index < node_stack.size())
            {
                auto fbx_node = node_stack[node_index];
                int32_t child_count = fbx_node->GetChildCount();
                for (int32_t child_index = 0; child_index < child_count; ++child_index)
                {
                    node_stack.emplace_back(fbx_node->GetChild(child_index));
                }
                int32_t attr_count = fbx_node->GetNodeAttributeCount();
                for (int32_t attr_index = 0; attr_index < attr_count; ++attr_index)
                {
                    auto node_attr = fbx_node->GetNodeAttributeByIndex(attr_index);
                    if (node_attr->GetAttributeType() == FbxNodeAttribute::EType::eMesh)
                    {
                        meshes.emplace_back(reinterpret_cast<FbxMesh*>(node_attr));
                    }
                }
            }
		}

        int64_t total_control_count = 0;
		for (FbxMesh* mesh : meshes)
		{
            int32_t     control_point_count = mesh->GetControlPointsCount();
            total_control_count = total_control_count + control_point_count;
		}
        out_mesh->m_vretices.reserve(total_control_count);
        auto& vertex_instances = out_mesh->m_vretices;
        for (FbxMesh* mesh : meshes)
        {
            int32_t     control_point_count = mesh->GetControlPointsCount();
            FbxVector4* control_points = mesh->GetControlPoints();
           
            for (int32_t control_point_idx = 0; control_point_idx < control_point_count; ++control_point_idx)
            {
                auto& cp = control_points[control_point_idx];

                SVertexInstance& vert = vertex_instances.emplace_back();
                memset(&vert, 0, sizeof(SVertexInstance));
                vert.position[0] = static_cast<float>(cp[0]);
                vert.position[1] = static_cast<float>(cp[1]);
                vert.position[2] = static_cast<float>(cp[2]);
            }

            if (!mesh->IsTriangleMesh())
            {
                auto converted_node = m_geometry_converter->Triangulate(mesh, true);
                if (converted_node != NULL && converted_node->GetAttributeType() == FbxNodeAttribute::eMesh)
                {
                    mesh = converted_node->GetNode()->GetMesh();
                }
                else
                {
                    printf("[error]:triangle failed!");
                }
            }

            int32_t polygon_count = mesh->GetPolygonCount();
            for (int32_t polygon_index = 0; polygon_index < polygon_count; ++polygon_index)
            {
                // mesh->GetPolygonVertex()
            }

        }
	}

	void CFbxImporter::ImportSkeletalMesh(CMesh* out_mesh)
	{

	}

	void CFbxImporter::ReleaseScene()
    {
	    if (m_fbx_scene)
	    {
            m_fbx_scene->Destroy();
            m_fbx_scene = nullptr;
	    }
    }
}
