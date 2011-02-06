/*
 * @file opencog/embodiment/Control/OperationalAvatarController/PsiFeelingUpdaterAgent.cc
 *
 * @author Zhenhua Cai <czhedu@gmail.com>
 * @date 2011-02-06
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include "OAC.h"
#include "PsiFeelingUpdaterAgent.h"

#include<boost/tokenizer.hpp>

using namespace OperationalAvatarController;

PsiFeelingUpdaterAgent::~PsiFeelingUpdaterAgent()
{

}

PsiFeelingUpdaterAgent::PsiFeelingUpdaterAgent()
{
    this->cycleCount = 0;

    // Force the Agent initialize itself during its first cycle. 
    this->forceInitNextCycle();
}

void PsiFeelingUpdaterAgent::init(opencog::CogServer * server) 
{
    logger().debug( "PsiFeelingUpdaterAgent::%s - Initializing the Agent [ cycle = %d ]",
                    __FUNCTION__, 
                    this->cycleCount
                  );

    // Get OAC
    OAC * oac = (OAC *) server;

    // Get AtomSpace
    const AtomSpace & atomSpace = * ( oac->getAtomSpace() );

    // Get petName
    const std::string & petName = oac->getPet().getName(); 

    // Get petHandle
    Handle petHandle = AtomSpaceUtil::getAgentHandle(atomSpace, petName); 

    if ( petHandle == Handle::UNDEFINED ) {
        logger().warn("PsiFeelingUpdaterAgent::%s - Failed to get the handle to the pet ( name = '%s' ) [ cycle = %d ]", 
                        __FUNCTION__, 
                        petName.c_str(), 
                        this->cycleCount
                     );
        return;
    }

    // Get Procedure repository
    const Procedure::ProcedureRepository & procedureRepository = 
                                               oac->getProcedureRepository();

    // Clear old feelingMetaMap; 
    this->feelingMetaMap.clear();

    // Get feeling names from the configuration file
    std::string feelingNames = config()["PSI_FEELINGS"];

    // Process feelings one by one
    boost::tokenizer<> feelingNamesTok (feelingNames);
    std::string feeling, feelingUpdater;
    FeelingMeta feelingMeta;

    for ( boost::tokenizer<>::iterator iFeelingName = feelingNamesTok.begin();
          iFeelingName != feelingNamesTok.end();
          iFeelingName ++ ) {

        feeling = (*iFeelingName);
        feelingUpdater = feeling + "FeelingUpdater";

        logger().debug( "PsiFeelingUpdaterAgent::%s - Searching the meta data of feeling '%s'.", 
                        __FUNCTION__, 
                        feeling.c_str() 
                      );

        // Search feeling updater
        // TODO: Load corresponding combo script within OAC
        if ( !procedureRepository.contains(feelingUpdater) ) {
            logger().warn( "PsiFeelingUpdaterAgent::%s - Failed to find '%s' in OAC's procedureRepository",
                           __FUNCTION__, 
                           feelingUpdater.c_str()
                         );
            continue;
        }
    
        // Get the corresponding EvaluationLink of the pet's feeling
        Handle evaluationLink = this->getFeelingEvaluationLink(server, feeling, petHandle);

        if ( evaluationLink == Handle::UNDEFINED )
        {
            logger().warn( "PsiFeelingUpdaterAgent::%s - Failed to get the EvaluationLink for feeling '%s'",
                           __FUNCTION__, 
                           feeling.c_str()
                         );

            continue;
        }

        // Insert the meta data of the feeling to feelingMetaMap
        feelingMeta.init(feelingUpdater, evaluationLink);
        feelingMetaMap[feeling] = feelingMeta;

        logger().debug( "PsiFeelingUpdaterAgent::%s - Store the meta data of feeling '%s' successfully.", 
                        __FUNCTION__, 
                        feeling.c_str() 
                      );
    }// for

    // Avoid initialize during next cycle
    this->bInitialized = true;
}

Handle PsiFeelingUpdaterAgent::getFeelingEvaluationLink(opencog::CogServer * server,
                                                        const std::string feelingName,
                                                        Handle petHandle)
{
    // Get the AtomSpace
    AtomSpace & atomSpace = * ( server->getAtomSpace() ); 

    // Get the Handle to feeling (PredicateNode)
    Handle feelingPredicateHandle = atomSpace.getHandle(PREDICATE_NODE, feelingName);

    if (feelingPredicateHandle == Handle::UNDEFINED) {
        logger().warn("PsiFeelingUpdaterAgent::%s - Failed to find the PredicateNode for feeling '%s' [ cycle = %d ].", 
                       __FUNCTION__, 
                       feelingName.c_str(), 
                       this->cycleCount
                      );

        return opencog::Handle::UNDEFINED; 
    }

    // Get the Handle to ListLink that contains the pet handle
    std::vector<Handle> listLinkOutgoing;

    listLinkOutgoing.push_back(petHandle);

    Handle listLinkHandle = atomSpace.getHandle(LIST_LINK, listLinkOutgoing);

    if (listLinkHandle == Handle::UNDEFINED) {
        logger().warn( "PsiFeelingUpdaterAgent::%s - Failed to find the ListLink containing the pet ( id = '%s' ) [ cycle = %d ].", 
                        __FUNCTION__, 
                        atomSpace.getName(petHandle).c_str(),
                        this->cycleCount
                      );

        return opencog::Handle::UNDEFINED; 
    } 

    // Get the Handle to EvaluationLink holding the pet's feeling
    std::vector<Handle> evaluationLinkOutgoing; 

    evaluationLinkOutgoing.push_back(feelingPredicateHandle);
    evaluationLinkOutgoing.push_back(listLinkHandle);

    Handle evaluationLinkHandle = atomSpace.getHandle(EVALUATION_LINK, evaluationLinkOutgoing);

    if (evaluationLinkHandle == Handle::UNDEFINED) {
        logger().warn( "PsiFeelingUpdaterAgent::%s - Failed to find the EvaluationLink holding the feling '%s' of the pet ( id = '%s' ) [ cycle = %d ].", 
                        __FUNCTION__, 
                        feelingName.c_str(), 
                        atomSpace.getName(petHandle).c_str(),
                        this->cycleCount
                     );

        return opencog::Handle::UNDEFINED; 
    } 

    return evaluationLinkHandle;

}

void PsiFeelingUpdaterAgent::runUpdaters(opencog::CogServer * server)
{
    logger().debug( "PsiFeelingUpdaterAgent::%s - Running feeling updaters (combo scripts) [ cycle = %d ]", 
                    __FUNCTION__ , 
                    this->cycleCount
                  );

    // Get OAC
    OAC * oac = (OAC *) server;

    // Get ProcedureInterpreter
    Procedure::ProcedureInterpreter & procedureInterpreter = oac->getProcedureInterpreter();

    // Get Procedure repository
    const Procedure::ProcedureRepository & procedureRepository = oac->getProcedureRepository();

    // Process feelings one by one
    std::map <std::string, FeelingMeta>::iterator iFeeling;

    std::string feeling, feelingUpdater;
    std::vector <combo::vertex> schemaArguments;
    Procedure::RunningProcedureID executingSchemaId;
    combo::vertex result; // combo::vertex is actually of type boost::variant <...>

    for ( iFeeling = feelingMetaMap.begin();
          iFeeling != feelingMetaMap.end();
          iFeeling ++ ) {

        feeling = iFeeling->first;
        feelingUpdater = iFeeling->second.updaterName;

        // Run the Procedure that update feeling and get the updated value
        const Procedure::GeneralProcedure & procedure = procedureRepository.get(feelingUpdater);

        executingSchemaId = procedureInterpreter.runProcedure(procedure, schemaArguments);

        // Wait until the end of combo script execution
        while ( !procedureInterpreter.isFinished(executingSchemaId) )
            procedureInterpreter.run(NULL);  

        // Check if the the updater run successfully
        if ( procedureInterpreter.isFailed(executingSchemaId) ) {
            logger().error( "PsiFeelingUpdaterAgent::%s - Failed to execute '%s'", 
                             __FUNCTION__, 
                             feelingUpdater.c_str() 
                          );

            iFeeling->second.bUpdated = false;

            continue;
        }
        else {
            iFeeling->second.bUpdated = true;
        }

        result = procedureInterpreter.getResult(executingSchemaId);

        // Store updated value to FeelingMeta.updatedValue
        // contin_t is actually of type double (see "comboreduct/combo/vertex.h") 
        iFeeling->second.updatedValue = get_contin(result);

        // TODO: Change the log level to fine, after testing
        logger().debug( "PsiFeelingUpdaterAgent::%s - The new level of feeling '%s' will be %f", 
                         __FUNCTION__, 
                         feeling.c_str(),
                         iFeeling->second.updatedValue                      
                      );
    }// for

}    

void PsiFeelingUpdaterAgent::setUpdatedValues(opencog::CogServer * server)
{
    logger().debug( "PsiFeelingUpdaterAgent::%s - Setting updated feelings to AtomSpace [ cycle =%d ]",
                    __FUNCTION__, 
                    this->cycleCount
                  );

    // Get OAC
    OAC * oac = (OAC *) server;

    // Get AtomSpace
    AtomSpace & atomSpace = * ( oac->getAtomSpace() );

    // Process feelings one by one
    std::map <std::string, FeelingMeta>::iterator iFeeling;
    std::string feeling;
    double updatedValue;
    Handle evaluationLink; 

    for ( iFeeling = feelingMetaMap.begin();
          iFeeling != feelingMetaMap.end();
          iFeeling ++ ) {

        if ( !iFeeling->second.bUpdated )
            continue;

        feeling = iFeeling->first;
        evaluationLink = iFeeling->second.evaluationLink;
        updatedValue = iFeeling->second.updatedValue;
       
        // Set truth value of corresponding EvaluationLink 
        SimpleTruthValue stvFeeling = SimpleTruthValue(updatedValue, 1.0);  
        atomSpace.setTV(evaluationLink, stvFeeling);

        // Reset bUpdated  
        iFeeling->second.bUpdated = false;

        // TODO: Change the log level to fine, after testing
        logger().debug( "PsiFeelingUpdaterAgent::%s - Set the level of feeling '%s' to %f", 
                         __FUNCTION__, 
                         feeling.c_str(),
                         updatedValue
                      );
    }// for
}

void PsiFeelingUpdaterAgent::sendUpdatedValues(opencog::CogServer * server)
{
    logger().debug( "PsiFeelingUpdaterAgent::%s - Sending updated feelings to the virtual world where the pet lives [ cycle =%d ]",
                    __FUNCTION__, 
                    this->cycleCount
                  );

    // Get OAC
    OAC * oac = (OAC *) server;

    // Get AtomSpace
    AtomSpace & atomSpace = * ( oac->getAtomSpace() );

    // Get petName
    const std::string & petName = oac->getPet().getName(); 

    // Prepare the data to be sent
    std::map <std::string, FeelingMeta>::iterator iFeeling;
    std::map <std::string, float> feelingValueMap; 
    std::string feeling;
    double updatedValue;

    for ( iFeeling = feelingMetaMap.begin();
          iFeeling != feelingMetaMap.end();
          iFeeling ++ ) {

        feeling = iFeeling->first;
        updatedValue = iFeeling->second.updatedValue;

        feelingValueMap[feeling] = updatedValue;        

    }// for

    // Send updated feelings to the virtual world where the pet lives
    oac->getPAI().sendEmotionalFeelings(petName, feelingValueMap);
}

void PsiFeelingUpdaterAgent::run(opencog::CogServer * server)
{
    this->cycleCount ++;

    logger().debug( "PsiFeelingUpdaterAgent::%s - Executing run %d times",
                     __FUNCTION__, 
                     this->cycleCount
                  );

    // Get OAC
    OAC * oac = (OAC *) server;

    // Get AtomSpace
    AtomSpace & atomSpace = * ( oac->getAtomSpace() );

    // Get petId
    const std::string & petId = oac->getPet().getPetId();

    // Check if map info data is available
    if ( atomSpace.getSpaceServer().getLatestMapHandle() == Handle::UNDEFINED ) {
        logger().warn( "PsiFeelingUpdaterAgent::%s - There is no map info available yet [ cycle = %d ]", 
                        __FUNCTION__, 
                        this->cycleCount
                     );
        return;
    }

    // Check if the pet spatial info is already received
    if ( !atomSpace.getSpaceServer().getLatestMap().containsObject(petId) ) {
        logger().warn( "PsiFeelingUpdaterAgent::%s - Pet was not inserted in the space map yet [ cycle = %d ]", 
                       __FUNCTION__, 
                       this->cycleCount
                     );
        return;
    }

    // Initialize the Agent (feelingMetaMap etc)
    if ( !this->bInitialized )
        this->init(server);

    // Run feeling updaters (combo scripts)
    this->runUpdaters(server);

    // Set updated values to AtomSpace
    this->setUpdatedValues(server);

    // Send updated values to the virtual world where the pet lives
    this->sendUpdatedValues(server); 
}
