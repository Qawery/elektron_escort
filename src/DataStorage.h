#ifndef ELEKTRON_ESCORT_DATASTORAGE_H
#define ELEKTRON_ESCORT_DATASTORAGE_H

#include <ros/ros.h>

class DataStorage
{
public:
    DataStorage();
    void UpdatePoseCooldowns(float timeElapsed);

    //Getters and Setters
    int GetMaxUsers();
    void SetMaxUsers(int _maxUsers);
    void SetPoseCooldownTime(float _poseCooldownTime);
    float GetPoseCooldownForUser(int _userNumber);

private:
    int maxUsers;
    float poseCooldownTime;
    std::vector<float> poseCooldownForUsers;
};

#endif //ELEKTRON_ESCORT_DATASTORAGE_H