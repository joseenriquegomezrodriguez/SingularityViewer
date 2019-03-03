/** 
* @file llskinningutil.cpp
* @brief  Functions for mesh object skinning
* @author vir@lindenlab.com
*
* $LicenseInfo:firstyear=2015&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2015, Linden Research, Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h"

#include "llskinningutil.h"
#include "llvoavatar.h"
#include "llviewercontrol.h"
#include "llmeshrepository.h"

// static
void LLSkinningUtil::initClass()
{
}

// static
U32 LLSkinningUtil::getMaxJointCount()
{
    U32 result = LL_MAX_JOINTS_PER_MESH_OBJECT;
	return result;
}

// static
U32 LLSkinningUtil::getMeshJointCount(const LLMeshSkinInfo *skin)
{
	return llmin((U32)getMaxJointCount(), (U32)skin->mJointNames.size());
}

// static
void LLSkinningUtil::scrubInvalidJoints(LLVOAvatar *avatar, LLMeshSkinInfo* skin)
{
    if (skin->mInvalidJointsScrubbed)
    {
        return;
    }
    for (U32 j = 0; j < skin->mJointNames.size(); ++j)
    {
        // Fix invalid names to "mPelvis". Currently meshes with
        // invalid names will be blocked on upload, so this is just
        // needed for handling of any legacy bad data.
        if (!avatar->getJoint(skin->mJointNames[j]))
        {
            LL_DEBUGS("Avatar") << "Mesh rigged to invalid joint" << skin->mJointNames[j] << LL_ENDL;
            skin->mJointNames[j] = "mPelvis";
        }
    }
    skin->mInvalidJointsScrubbed = true;
}

// static
void LLSkinningUtil::initSkinningMatrixPalette(
    LLMatrix4a* mat,
    S32 count, 
    const LLMeshSkinInfo* skin,
    LLVOAvatar *avatar,
	bool relative_to_avatar)
{
	for (U32 j = 0; j < (U32)count; ++j)
	{
		LLJoint *joint = NULL;
		if( skin->mJointNums[j] <= -50 )
		{
			// Give up silently.
			mat[j].loadu((F32*)skin->mInvBindMatrix[j].mMatrix);
			continue;
		}
		else if (skin->mJointNums[j] < 0 )
		{
			joint = avatar->getJoint(skin->mJointNames[j]);
			if (!joint)
			{
				LL_WARNS_ONCE("Avatar") << "Rigged to invalid joint name " << skin->mJointNames[j] << " Using 'root' fallback." << LL_ENDL;
				joint = avatar->getJoint("mRoot");
			}
			if (joint)
			{
				//LL_INFOS() << "Found joint " << skin->mJointNames[j] << " Id = " << joint->getJointNum() << LL_ENDL;
				skin->mJointNums[j] = joint->getJointNum();
			}
			else
			{
				--skin->mJointNums[j];
			}
		}
		else
		{
			joint = avatar->getJoint(skin->mJointNums[j]);
			if (!joint)
			{
				LL_WARNS_ONCE() << "Joint not found: " << skin->mJointNames[j] << " Id = " << skin->mJointNums[j] << LL_ENDL;
			}
		}
		if (joint)
		{
			LLMatrix4a bind;
			bind.loadu((F32*)skin->mInvBindMatrix[j].mMatrix);
			if (relative_to_avatar)
			{
				LLMatrix4a trans = joint->getWorldMatrix();
				trans.translate_affine(avatar->getPosition() * -1.f);
				mat[j].setMul(trans, bind);
			}
			else
				mat[j].setMul(joint->getWorldMatrix(), bind);
		}
		else
		{
			mat[j].loadu((F32*)skin->mInvBindMatrix[j].mMatrix);
			// This  shouldn't  happen   -  in  mesh  upload,  skinned
			// rendering  should  be disabled  unless  all joints  are
			// valid.  In other  cases of  skinned  rendering, invalid
			// joints should already have  been removed during scrubInvalidJoints().
			LL_WARNS_ONCE("Avatar") << "Rigged to invalid joint name " << skin->mJointNames[j] << LL_ENDL;
		}
	}
}

// static
void LLSkinningUtil::checkSkinWeights(const LLVector4a* weights, U32 num_vertices, const LLMeshSkinInfo* skin)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	const S32 max_joints = skin->mJointNames.size();
    for (U32 j=0; j<num_vertices; j++)
    {
        const F32 *w = weights[j].getF32ptr();
            
        F32 wsum = 0.0;
        for (U32 k=0; k<4; ++k)
        {
            S32 i = llfloor(w[k]);
            llassert(i>=0);
            llassert(i<max_joints);
            wsum += w[k]-i;
        }
        llassert(wsum > 0.0f);
    }
#endif
}

void LLSkinningUtil::scrubSkinWeights(LLVector4a* weights, U32 num_vertices, const LLMeshSkinInfo* skin)
{
    const S32 max_joints = skin->mJointNames.size();
    for (U32 j=0; j<num_vertices; j++)
    {
        F32 *w = weights[j].getF32ptr();

        for (U32 k=0; k<4; ++k)
        {
            S32 i = llfloor(w[k]);
            F32 f = w[k]-i;
            i = llclamp(i,0,max_joints-1);
            w[k] = i + f;
        }
    }
	checkSkinWeights(weights, num_vertices, skin);
}

// static
void LLSkinningUtil::getPerVertexSkinMatrix(
    const F32* weights,
    LLMatrix4a* mat,
    bool handle_bad_scale,
    LLMatrix4a& final_mat,
    U32 max_joints)
{
    bool valid_weights = true;
	final_mat.clear();

	S32 idx[4];

	LLVector4 wght;

	F32 scale = 0.f;
	for (U32 k = 0; k < 4; k++)
	{
		F32 w = weights[k];

		idx[k] = (S32) w;

		wght[k] = w - idx[k];
		scale += wght[k];
	}
	if (handle_bad_scale && scale <= 0.f)
	{
		wght = LLVector4(F32_MAX,F32_MAX,F32_MAX,F32_MAX);
		valid_weights = false;
	}
	else
	{
		// This is enforced  in unpackVolumeFaces()
		llassert(scale>0.f);

		wght *= 1.f/scale;
	}

	for (U32 k = 0; k < 4; k++)
	{
		F32 w = wght[k];

		LLMatrix4a src;
		src.setMul(mat[idx[k]], w);

		final_mat.add(src);
	}
	// SL-366 - with weight validation/cleanup code, it should no longer be
	// possible to hit the bad scale case.
	llassert(valid_weights);
}

