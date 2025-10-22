#include "pch.h"
#include "editor.h"
#include "entitymgr.h"
#include "filemgr.h"
#include "interface.h"
#include "defines.h"
#include <CRenderer.h>
#include <CModelInfo.h>
#include <filesystem>

float EntityMgr::GetBoundingBoxGroundZ(CObject *pObj)
{
    if (!pObj)
    {
        return 0.0f;
    }
    CMatrix *matrix = pObj->GetMatrix();
    CColModel *pColModel = pObj->GetColModel();
    CVector min = pColModel->m_boundBox.m_vecMin;
    CVector max = pColModel->m_boundBox.m_vecMax;

    CVector workVec = min;

    workVec.x = max.x;
    workVec.y = max.y;
    CVector ground = *matrix * workVec;

    return ground.z;
}

EntityMgr EntMgr;
EntityMgr::EntityMgr()
{
    // ---------------------------------------------------
    static std::string ideName = "Unknown";
    CdeclEvent<AddressList<0x5B8428, H_CALL>, PRIORITY_BEFORE, ArgPick2N<char *, 0, char *, 1>, void(char *a1, char *a2)> fileLoader;
    fileLoader += [this](char *a1, char *a2)
    {
        ideName = std::filesystem::path(a1).stem().string();
    };

    CdeclEvent<AddressList<0x5B85DD, H_CALL>, PRIORITY_AFTER, ArgPickN<char *, 0>, void(char *str)> loadIdeEvent;
    loadIdeEvent += [this](char *str)
    {
        int model, unk2;
        char dffName[32], txdName[32];
        float unk;
        sscanf(str, "%d %s %s %f %d", &model, dffName, txdName, &unk, &unk2);
        if (std::find(m_IdeList.begin(), m_IdeList.end(), ideName) == m_IdeList.end())
        {
            m_IdeList.push_back(ideName);
        }
        m_NameList[m_IdeList.back()][model] = std::string(dffName);
        m_nTotalEntities++;
    };

    CdeclEvent<AddressList<0x5B864C, H_CALL>, PRIORITY_AFTER, ArgPickN<char *, 0>, int(char *)> hkVehicleParseEvent;
    m_IdeList.push_back("vehicles");
    hkVehicleParseEvent += [this](char *line)
    {
        int model;
        char v26[24], txdName[24], v16[24], vehicleName[24], a2[24], v25[24], v18[24];
        int v20, v22, v21, v13, v19;
        float v15, v14;
        sscanf(line, "%d %s %s %s %s %s %s %s %d %d %x %d %f %f %d", &model, v26, txdName, v16, vehicleName, a2, v25,
               v18, &v20, &v22, &v21, &v13, &v15, &v14, &v19);

        m_NameList["vehicles"][model] = std::string(vehicleName);
        m_nTotalEntities++;
    };
}

std::string EntityMgr::FindNameFromModel(int model)
{
    for (auto &data : m_NameList)
    {
        for (auto &ideData : data.second)
        {
            if (ideData.first == model)
            {
                return ideData.second;
            }
        }
    }
    return "dummy";
}