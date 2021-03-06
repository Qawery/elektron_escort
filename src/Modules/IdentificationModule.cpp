#include "IdentificationModule.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//Public
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IdentificationModule::Initialize(ros::NodeHandle *nodeHandlePrivate) {
    int _logLevel;
    if(!nodeHandlePrivate->getParam("identificationModuleLogLevel", _logLevel)) {
        ROS_WARN("IdentificationModule: Log level not found, using default");
        logLevel = DEFAULT_IDENTIFICATION_MODULE_LOG_LEVEL;
    }
    else {
        switch (_logLevel) {
            case 0:
                logLevel = Debug;
                break;
            case 1:
                logLevel = Info;
                break;
            case 2:
                logLevel = Warn;
                break;
            case 3:
                logLevel = Error;
                break;
            default:
                ROS_WARN("IdentificationModule: Requested invalid log level, using default");
                logLevel = DEFAULT_IDENTIFICATION_MODULE_LOG_LEVEL;
                break;
        }
    }
    if(!nodeHandlePrivate->getParam("identificationThreshold", identificationThreshold)) {
        if(logLevel <= Warn) {
            ROS_WARN("IdentificationModule: Value of identificationThreshold not found, using default: %f", DEFAULT_IDENTIFICATION_THRESHOLD);
        }
        identificationThreshold = DEFAULT_IDENTIFICATION_THRESHOLD;
    }
    double methodTrustValue;
    methods[IM_UserId] = new UserID_Method();
    if(!nodeHandlePrivate->getParam("userID_MethodTrust", methodTrustValue)) {
        if(logLevel <= Warn) {
            ROS_WARN("IdentificationModule: Trust value for userID method not found, using default: %f", DEFAULT_USER_ID_METHOD_TRUST);
        }
        methods[IM_UserId]->SetTrustValue(DEFAULT_USER_ID_METHOD_TRUST);
    }
    else {
        methods[IM_UserId]->SetTrustValue(methodTrustValue);
    }
    methods[IM_Height] = new Height_Method();
    if(!nodeHandlePrivate->getParam("height_MethodTrust", methodTrustValue)) {
        if(logLevel <= Warn) {
            ROS_WARN("IdentificationModule: Trust value for height method not found, using default: %f", DEFAULT_USER_ID_METHOD_TRUST);
        }
        methods[IM_Height]->SetTrustValue(DEFAULT_HEIGHT_METHOD_TRUST);
    }
    else {
        methods[IM_UserId]->SetTrustValue(methodTrustValue);
    }
    if(logLevel <= Info) {
        ROS_INFO("IdentificationModule: Initialized");
    }
    state = NoTemplate;
    return true;
}

void IdentificationModule::Update()
{
    switch (state) {
        case IdentificationStates::NoTemplate:
            break;
        case IdentificationStates::SavingTemplate:
            ContinueSavingTemplate();
            break;
        case IdentificationStates::PresentTemplate:
            IdentifyUser();
            break;
    }
}

void IdentificationModule::Finish() {
    for( int i=0; i < IM_NUMBER_OF_METHODS; ++i) {
        delete methods[i];
    }
}

void IdentificationModule::ClearTemplate() {
    for( int i=0; i < IM_NUMBER_OF_METHODS; ++i) {
        methods[i]->ClearTemplate();
    }
    state = IdentificationStates::NoTemplate;
    if(logLevel <= Info) {
        ROS_INFO("IdentificationModule: Template cleared");
    }
}

void IdentificationModule::SaveTemplateOfCurrentUser() {
    state = IdentificationStates::SavingTemplate;
    for( int i=0; i < IM_NUMBER_OF_METHODS; ++i) {
        methods[i]->BeginSaveTemplate();
    }
    if(logLevel <= Info) {
        ROS_INFO("IdentificationModule: Begin saving template");
    }
}

IdentificationStates IdentificationModule::GetState() {
    return state;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//Private
///////////////////////////////////////////////////////////////////////////////////////////////////////////
void IdentificationModule::ContinueSavingTemplate() {
    MethodState templateState = Ready;
    if(DataStorage::GetInstance().IsPresentOnScene(DataStorage::GetInstance().GetCurrentUserXnId())) {
        for (int i = 0; i < IM_NUMBER_OF_METHODS; ++i) {
            methods[i]->ContinueSaveTemplate();
            if (methods[i]->GetState() == CreatingTemplate) {
                templateState = CreatingTemplate;
            } else if (methods[i]->GetState() == NotReady) {
                templateState = NotReady;
                break;
            }
        }
    }
    else {
        templateState = NotReady;
    }
    if(templateState == Ready) {
        state = IdentificationStates::PresentTemplate;
        if(logLevel <= Info) {
            ROS_INFO("IdentificationModule: Saving template successful");
        }
    }
    else if (templateState == NotReady) {
        if(logLevel <= Info) {
            ROS_INFO("IdentificationModule: Saving template failed");
        }
        ClearTemplate();
    }
}

void IdentificationModule::IdentifyUser() {
    for( int i=0; i < IM_NUMBER_OF_METHODS; ++i) {
        methods[i]->Update();
    }
    XnUserID previousUser = DataStorage::GetInstance().GetCurrentUserXnId();
    std::set<XnUserID>* presentUsers = DataStorage::GetInstance().GetPresentUsersSet();
    if(presentUsers->size()>0) {
        std::set<XnUserID>::iterator iter;
        int index = 0;
        float usersRanking[presentUsers->size()];
        XnUserID usersIds[presentUsers->size()];
        for (iter = presentUsers->begin(); iter != presentUsers->end(); ++iter) {
            usersIds[index] = *iter;
            usersRanking[index] = 0.0;
            ++index;
        }
        for (index = 0; index < presentUsers->size(); ++index) {
            for (int i = 0; i < IM_NUMBER_OF_METHODS; ++i) {
                usersRanking[index] += (methods[i]->RateUser(usersIds[index]) * methods[i]->GetTrustValue());
            }
        }
        int bestMatchingUserIndex = 0;
        for (index = 1; index < presentUsers->size(); ++index) {
            if (usersRanking[bestMatchingUserIndex] < usersRanking[index]) {
                bestMatchingUserIndex = index;
            }
        }
        if (usersRanking[bestMatchingUserIndex] >= DEFAULT_IDENTIFICATION_THRESHOLD) {
            DataStorage::GetInstance().SetCurrentUserXnId(usersIds[bestMatchingUserIndex]);
            if(previousUser != usersIds[bestMatchingUserIndex]) {
                if (logLevel <= Info) {
                    ROS_INFO("IdentificationModule: Switched to user %d", usersIds[bestMatchingUserIndex]);
                }
            }
        } else {
            DataStorage::GetInstance().SetCurrentUserXnId(NO_USER);
        }
    }
    else {
        DataStorage::GetInstance().SetCurrentUserXnId(NO_USER);
    }
    for( int i=0; i < IM_NUMBER_OF_METHODS; ++i) {
        methods[i]->LateUpdate();
    }
}