#pragma once
#include <string>

#include <CVector.h>
#include <CQuaternion.h>
#include <CObject.h>
#include <CPools.h>

/*
  EntityInfo MapEditor class
  Stores MapEditor object specific data
*/
class EntityInfo {
  public:
    CVector m_Euler = { 0, 0, 0 };
    int m_nHandle = NULL; 
    CObject *m_pObj = nullptr; 
    std::string m_sModelName = "";

    EntityInfo(CObject *obj);
    CVector GetEuler();
    void SetEuler(CVector rot);
    CQuaternion GetQuat() const;
    void SetQuat(CQuaternion quat);
};