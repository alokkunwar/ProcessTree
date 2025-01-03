#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
#include<stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#define LENGTH_OF_PATH 512
#define SIZE_OF_BUFFER 1024
int globalCounter = 0;
int globalNonDirectCounter = 0;
int globalGrandChildCounter = 0;
int inputProcessId = 0;
int inputRootProcessId = 0;
int validateInputWithNoOption = 0;
void validateInput(int numberOfInputs, char *inputValues[]);
void exitMethod();
void displayMessageToUser(char *displayMessage);
int setInputVariable(char *processId);
void validateProcessId(int processId, int isRootProcessId);
int getRootProcessId(int providedProcessId, int providedRootProcessId);
int isBelongsToProcessTree(int retriveddProcessId, int providedProcessId, int providedRootProcessId);
void listProcessId(int providedProcessId, int providedRootProcessId);
void performActionIfNoOptionProveided(int providedProcessId, int providedRootProcessId);
void validateOption(char *option, int providedProcessId, int providedRootProcessId);
void makeAProcessPause(long int providedProcessId);
void sendStopSignalToDescendants(int providedProcessId, int providedRootProcessId);
typedef enum 
{
  GETDIRECT,
  GETALLSIBLING,
  GETNONDIRECT,
  GETGRANDCHILDREN
}OperationType;
int allDirectChild[500];
int allNonDirectChild[500];
int allSibling[500];
int allGrandChild[500];
int listOfAllDescendent[200] = {0};
char * getPath(char * format, int optionalParm, int isChildren);
void printMessageonFileHandling(char * message);
int openFile(const char *filePathname, int flag);
void closeFile(int fileDescriptor);
bool isAZombieProcess(char * token);
bool getProcessStatus(int processId, int flag);
int retriveTheParentId(int providedProcessId);
void getChildren(int parentId, OperationType operation);
void storeChildren(OperationType operation, char *listOfToken, int counter);
void displayDefunctSibling(int providedProcessId) ;
void getNonImmediateDescendants(int processId,  OperationType operation);
void displayListofSiblings(int listOfValues[], char* exitMessage, int flag);
void killParentOrListZombieProcess(int providedProcessId, int isList);
bool isPausedProcess(char * token);
void removeFile(char * commandToRemove);
void sendSIGSTOPOrSIGKILLSignal(int providedProcessId, int flag);
void retriveAllSiblingProcess(int providedProcessId);
void sendSIGCONTSignal(int providedRootProcessId) ;
int prvProcessId;
void printDoesNotBelongsMessage();


//Method to create the path for files like stat, status (/proc/%d/status) etc 
char * getPath(char * format, int optionalParm, int isChildren)
{
  static char stringBuffer[LENGTH_OF_PATH];
  //stringBuffer is used to store the path which is in format variable and it is being resolved using optionalParm
  if(isChildren)
  snprintf(stringBuffer, LENGTH_OF_PATH, format, optionalParm, optionalParm);
  else
  snprintf(stringBuffer, LENGTH_OF_PATH, format, optionalParm);

// returning the value.
  return stringBuffer;
}

//Method to display message to user and exit. This is mostly being called if there is any issue with file opening.
void printMessageonFileHandling(char * message)
{
  displayMessageToUser(message);
  exitMethod();
}

//Method to open the provided file (e.g /proc/%d/status) and handle error in case.
int openFile(const char *filePathname, int flag)
{
  int fileDescriptor = open(filePathname, flag);
  if(fileDescriptor == -1)
  {
    validateInputWithNoOption == 1 ?  "" : 
    printMessageonFileHandling("Error while opening the file.");
    validateInputWithNoOption = 0;
  }

  //If sucessful, returing the fileDescriptor
  return fileDescriptor;
}

// Method to close the file. 
void closeFile(int fileDescriptor)
{
  close(fileDescriptor);
}

// Method to check if a given string contains Z (zombie). Mainly, used to check if a process is zombie or not.
bool isAZombieProcess(char * token)
{
  return strstr(token, "Z (zombie)") != NULL ? true : false;
}

// Method to check if a given string contains T (stopped). Mainly, used to check if a process is paused or not.
bool isPausedProcess(char * token)
{
  return strstr(token, "T (stopped)") != NULL ? true : false;
}

//Method to get the status of any provided process.
bool getProcessStatus(int processId, int checkZombie) 
{
  // Preparing the status file path and opening it to read the status.
  char * fileLocation = getPath("/proc/%d/status", processId, 0);
  int fileDescriptor = openFile(fileLocation, O_RDONLY);

  char spaceToReadFile[SIZE_OF_BUFFER];
  long int readDescriptor;
  //Reading the status file
  while ((readDescriptor = read(fileDescriptor, spaceToReadFile, sizeof(spaceToReadFile))) > 0) 
  {
    spaceToReadFile[readDescriptor] = '\0';
    char *listOfToken = strtok(spaceToReadFile, "\n");
    while(listOfToken != NULL) 
    {
      //Checking if the token contains "State:"
      if(strstr(listOfToken, "State:") != NULL)
      {
        // if method is being called to check Zombie process then calling isAZombieProcess() else calling isPausedProcess()
        bool processStatus = checkZombie ? isAZombieProcess(listOfToken) : isPausedProcess(listOfToken);
        closeFile(fileDescriptor);
        // returing the process status
        return processStatus;
      }
      listOfToken = strtok(NULL, "\n");
    }
  
  }

 return false;
}

//Method to get the parent Id.
int retriveTheParentId(int providedProcessId) 
{
  //Prepaaring the path for the stat file and opening the file to read.
  char * fileLocation = getPath("/proc/%d/stat", providedProcessId, 0);
  int fileDescriptor = openFile(fileLocation, O_RDONLY);
  char spaceToReadFile[SIZE_OF_BUFFER];
  int parentId = -1;
  //Reading the file.
  if (read(fileDescriptor, spaceToReadFile, sizeof(spaceToReadFile)) > 0) 
  {
      //Extracting the Parent id and storing it in parentId
      sscanf(spaceToReadFile, "%*d %*s %*s %d", &parentId);
  }

  closeFile(fileDescriptor);

  //returning the parent Id
  return parentId;
}


// Method used to store diffent type of child to perform some processing.
void storeChildren(OperationType operation, char *listOfToken, int counter)
{

    if(operation == GETDIRECT)
    {
      // Direct child is being stored in the allDirectChild
      allDirectChild[counter] = atoi(listOfToken);
    }
    else if(operation == GETALLSIBLING)
    {
      // All sibling is being stored in the allSibling
      allSibling[counter] = atoi(listOfToken);
    }
    else if(operation == GETNONDIRECT)
    {
      // All Non Direct Child is being stored in the allNonDirectChild
      allNonDirectChild[globalNonDirectCounter++] = atoi(listOfToken);
      //listOfAllDescendent is being used to retrive all the descendent of a given process. Also being set at getNonImmediateDescendants.
      //globalCounter is being used to store the number of element in  listOfAllDescendent efficiently.
      listOfAllDescendent[globalCounter++] = atoi(listOfToken);

    }
    else if(operation == GETGRANDCHILDREN)
    {
      //Used to store all the Grand child.
      allGrandChild[globalGrandChildCounter++] = atoi(listOfToken);
    }
}

//Method to get the children of a given process Id
void getChildren(int parentId, OperationType operation) 
{
  //Preparing the path and opening the file to read.
  char * fileLocation = getPath("/proc/%d/task/%d/children", parentId, 1);
  int fileDescriptor = openFile(fileLocation, O_RDONLY);
  
  char spaceToReadFile[SIZE_OF_BUFFER];
  long int readDescriptor;
  int counter = 0;
  //Reading the file.
  while ((readDescriptor = read(fileDescriptor, spaceToReadFile, sizeof(spaceToReadFile))) > 0) 
  {
    spaceToReadFile[readDescriptor] = '\0';
    char *listOfToken = strtok(spaceToReadFile, " ");
    while(listOfToken != NULL) 
    {
      // Calling the method to store the respective child(Direct, non-direct, grand, All sibling etc)
      //After this call all the child for the respective operation will be stored in the respective arrays.
      storeChildren(operation, listOfToken, counter);
      counter++;
      listOfToken = strtok(NULL, " ");
    }  
  }
  if(readDescriptor < 0)
    displayMessageToUser("No Direct children");

  closeFile(fileDescriptor);
}

// Have written a general method to show the siblings. It mainly used to show Non-Direct, Direct, Grand and All the siblings.
void displayListofSiblings(int *listOfValues, char* exitMessage, int isDisplaySibling)
{

  int size =  sizeof(listOfValues);
  if(size > 0) //If Size > 0, means it has some value, going to print it.
  {
    int firstValue = listOfValues[0];
    
    // Below Condition is used to print the initial message to the user.
    if(firstValue != 0)
    {
      // This is wrttiren to handle some edge cases to display a proper message to the user.
       if(isDisplaySibling)
        {
          if(firstValue != prvProcessId)
            displayMessageToUser("Below are the process:");
          else
          {
              listOfValues[1] > 0 &&  listOfValues[1] != prvProcessId ? displayMessageToUser("Below are the process:") : displayMessageToUser(exitMessage);

          }
        }
        else
          displayMessageToUser("Below are the process:");    
    }
     else
      displayMessageToUser(exitMessage);
    //Now after the initial message, iterating the array to print the value.  
    for(int counter = 0; counter < size; counter++) 
    {
      int prcessID = listOfValues[counter];
			if(prcessID == 0) 
      {
				break;
			}
			else 
      {
        //Below condition is used to show all the siblings.
        if(isDisplaySibling)
        {
          if(prcessID != prvProcessId)
          	printf("%d\n", prcessID);

        }
        else 
				printf("%d\n", prcessID); //Mainly used to show other types of like Direct, non-direct and grand child.
			}
		}
  }
  else
  {
    displayMessageToUser(exitMessage); //Display exit message in case if not exist.
  }
}

//Below method is used to display the Defunct siblings.
void displayDefunctSibling(int providedProcessId) 
{

  //Retriving the parent id of the given process id so that we can get all the child of the parent and then will check the status.
  int parentId = retriveTheParentId(providedProcessId);

  //Calling the method to get all the children of the parent process.
  getChildren(parentId, GETALLSIBLING);
  int size =  sizeof(allSibling);

  // tempAllSibling is being used to store all the sibling of the  provided process id.
  int tempAllSibling[100] = {0};
  int tempAllSiblingCounter = 0;
  for(int counter = 0; counter < size; counter++)
  {
      if(allSibling[counter] != providedProcessId)
        tempAllSibling[tempAllSiblingCounter++] = allSibling[counter];

  }

  //tempAllSibling has all the sibling of providedProcessId and tempAllSiblingCounter represent the size of tempAllSibling
  if(tempAllSiblingCounter > 0)
  {
    int firstValue = tempAllSibling[0];
    //Checking the condition to display the initial message.
    firstValue != 0 && firstValue != providedProcessId ? displayMessageToUser("Below are the Zombies process:") : displayMessageToUser("There is no sibling which is Zombie.");

    // iterating all the sibiling to check its status
    for(int counter = 0; counter < tempAllSiblingCounter; counter++) 
    {
      int prcessID = tempAllSibling[counter];
			if(prcessID == 0) 
      {
				break;
			}
			else if(prcessID != providedProcessId && getProcessStatus(prcessID, 1)) 
      {
        //getProcessStatus returns if the process is defunct or not. if defunct then print the process id.
				printf("%d\n", prcessID);
			}
		}
  }
  else
  {
    displayMessageToUser("There is no sibling which is Zombie."); //In case no defunct sibling.
  }
}

// This method is used to get the Non Immediate Descendants of the given process id based on the type of operation(Grand children and non direct children)
void getNonImmediateDescendants(int processId, OperationType operation) 
{
  //Getting the file path and opening it to read.
  char * fileLocation = getPath("/proc/%d/task/%d/children", processId, 1);
  int fileDescriptor = openFile(fileLocation, O_RDONLY);
  
  char spaceToReadFile[SIZE_OF_BUFFER];
  long int readDescriptor;

  //Reading the file.
  while ((readDescriptor = read(fileDescriptor, spaceToReadFile, sizeof(spaceToReadFile))) > 0) 
  {
    spaceToReadFile[readDescriptor] = '\0';
    char *listOfToken = strtok(spaceToReadFile, " ");
    while(listOfToken != NULL) 
    {
      // listOfAllDescendent is being used to store the all the direct decendents of the provided process id.
      //Later on I am going to iterate it to get all the childrens of these direct decendents to get the non direct decendents.
      listOfAllDescendent[globalCounter++] = atoi(listOfToken);
      listOfToken = strtok(NULL, " ");
    }  
  }
  closeFile(fileDescriptor);

  int position = 0;
  while(1)
  {
    // Non exit condition for the while loop.
    if(position != globalCounter)
    {
      if(operation == GETGRANDCHILDREN)
      {
        getChildren(listOfAllDescendent[position], GETGRANDCHILDREN); //If operation is GETGRANDCHILDREN, call getChildren to get  all the direct decendents of the process Id  that I collected in listOfAllDescendent above. By this, I'll get all the non direct decendents of processId
      }
      else if(operation == GETNONDIRECT)
      {
        getChildren(listOfAllDescendent[position], GETNONDIRECT);//If operation is GETNONDIRECT, call getChildren to get  all the direct decendents of the process Id  that I collected in listOfAllDescendent above. By this, I'll get all the non direct decendents of processId
      }

      position++;
    }
    else
      break;// exit condition.
      
  }
}

//Method to kill the provided process Id using SIGKILL
void callToKillAProcess(int processID) 
{
	if (processID > 0) 
  {
      if (kill(processID, SIGKILL) == 0) 
      {
          printf("Process Id %d has been killed successfully.\n", processID);
      } 
      else 
      {
          displayMessageToUser("An issue has been occured while killing the Process Id.");
      }
  }
}

// Mainly used to kill the parent of a Zombie process(-KZ command) and also used to  list all the Zombie process.
void killParentOrListZombieProcess(int providedProcessId, int isListDefunct) 
{
  //Calling this method to get all the Descendent and then I will iterate all the Descendent and will do the required operation eg. list or kill.
	getNonImmediateDescendants(providedProcessId, GETNONDIRECT);
  int size  =  sizeof(listOfAllDescendent);
  int printDisplayMessage = 1;

	if(size > 0)
  {
    //iterating over all the descendent
		for(int iteration = 0; iteration < sizeof(listOfAllDescendent); iteration++) 
    {
			if(listOfAllDescendent[iteration] == 0) 
      {
				break;
			}
      //In below condition, calling getProcessStatus to check if the process is Zombie or not
			if(getProcessStatus(listOfAllDescendent[iteration], 1))  
      {
        //If the operation is to list the Zombie process then it will enter into the below condition.
        if(isListDefunct)
        {
          if(printDisplayMessage)
          {
            displayMessageToUser("Below are the process:");
            printDisplayMessage = 0;
          }
          printf("%d\n",listOfAllDescendent[iteration]); // listing all the Zombies.
        }	
        else
        {    
          // If the operation is to kill a Zombie process. In below, retriving the parent of the zombie to kill.
          int parentId = retriveTheParentId(listOfAllDescendent[iteration]);
          //Calling to kill the parent of the Zombie.
          callToKillAProcess(parentId);
          if(parentId > 0)
           printDisplayMessage = 0;

        }
			}
		}
	} 

  if(printDisplayMessage)
    displayMessageToUser("There is no descendant defunct (Zombie) processes.");

}

//Below method is used to send SIGCONT Signal to resume all the decendents of the provided process id which are already stopped.
void sendSIGCONTSignal(int providedRootProcessId) 
{
  //Calling the method to get all the descendents.
	getNonImmediateDescendants(providedRootProcessId, GETNONDIRECT);
  //Iterating over all the descendents, and if it is paused then resume it.
  for(int counter = 0; counter < sizeof(listOfAllDescendent); counter++)
  {
    if(listOfAllDescendent[counter] == 0)
      break;
    // retriving the status of the process.
    bool isPaused = getProcessStatus(listOfAllDescendent[counter], 0);
    if(isPaused)
    {
      kill(listOfAllDescendent[counter], SIGCONT); // Sending the SIGCONT signal to resume it.
      printf("Process Id %d has been resumed.\n", listOfAllDescendent[counter]);
    }
  }

}

//Method to send SIGSTOP Or SIGKILL Signal to all descendents.
void sendSIGSTOPOrSIGKILLSignal(int providedProcessId, int processPause)
{
  //Calling method to get all the descendents.
	getNonImmediateDescendants(providedProcessId, GETNONDIRECT);

  // Iterating all the descendents to send SIGSTOP Or SIGKILL signal based on the requirement.
  for(int i = 0; i< sizeof(listOfAllDescendent); i++)
  {
    if(listOfAllDescendent[i] == 0) 
    {
      break;
    }

    if(listOfAllDescendent[i] != providedProcessId)
    {
      // based on the variable processPause, I am decideing to pause or Kill the process.
      processPause == 1 ? makeAProcessPause(listOfAllDescendent[i]) : callToKillAProcess(listOfAllDescendent[i]);
    }
  }
}

// Below method is to retrive all the siblings.
void retriveAllSiblingProcess(int providedProcessId) 
{
  // Calling this method to get the parent of the process id. and then calling getChildren to get all the children of the parent id.
	int parentId = retriveTheParentId(providedProcessId);
  getChildren(parentId, GETALLSIBLING);
  //calling the method to display all the siblings.
  displayListofSiblings(allSibling, "There is no sibling of the provided Process Id.", 1);
}

//Caliing the method to get the root process Id of a given process.
int getRootProcessId(int providedProcessId, int providedRootProcessId) 
{
  // using the temp variable processId.
  int processId = providedProcessId;
  while (processId != providedRootProcessId && processId > 1) 
  {
      // Now calling the method to get the parent id of the process id and updating the temp variable to know the root process id.
      processId = retriveTheParentId(processId);
  }

// returing the root process id.
  return processId;
}

// This method I have wriiten to display the list of parent Ids of the given process id to handle the case if the option is not provided.
void listProcessId(int providedProcessId, int providedRootProcessId) 
{
  int processId = providedProcessId;
  int printDisplayMessage = 1;
  int printErrorMessage = 0;
  while (processId != providedRootProcessId && processId > 1) 
  {
    // Condinton to display the initial message.
    if(printDisplayMessage)
    {
      printDisplayMessage = 0;
      displayMessageToUser("List of PID : ");
    }

    //Print the process Id and calling a method to get its parent id.
    printf("%d\n",processId);
    processId = retriveTheParentId(processId);
  }
if(printDisplayMessage)
  printDoesNotBelongsMessage();
}

// Method to print the message if the provided process id does not belongs to the process tree.
void printDoesNotBelongsMessage()
{
   printf("The process id %d does not belong to the tree rooted at %d\n", inputProcessId, inputRootProcessId);
}

// Method to check if the process belongs to the process tree or not.
int isBelongsToProcessTree(int retriveddProcessId, int providedProcessId, int providedRootProcessId)
{
  if (retriveddProcessId != providedRootProcessId) 
  {
    printDoesNotBelongsMessage();
    return 0;
  }

  return 1;
}

// General method to guide the user to provide input in proper format.
void validateInput(int numberOfInputs, char *inputValues[])
{

  if(numberOfInputs != 3 && numberOfInputs != 4)
  {
    displayMessageToUser("Please provide input in the format : [Option] [root_process] [process_id]");
    displayMessageToUser("Please provide input in the format : [root_process] [process_id]");
    exitMethod();
  }
}

//Method to exit.
void exitMethod()
{
 exit(EXIT_FAILURE);
}

// Method to display a message to user.
void displayMessageToUser(char *displayMessage)
{
  printf(displayMessage);
  printf("\n");
}

//Method to convert a string into integer.
int setInputVariable(char *processId)
{
  return atoi(processId);
}

// General message to check if the provided Root process id or the process id is valid or not.
void validateProcessId(int processId, int isRootProcessId)
{
  if(processId < 1)
  {
    isRootProcessId > 0 ? displayMessageToUser("Root process id is not valid.") : displayMessageToUser("Process id is not valid.");
    exitMethod();
  }

}

// Method used to handle the case when no option is provided.
// In this we have to print the process and all its parents except the root process.
void performActionIfNoOptionProveided(int providedProcessId, int providedRootProcessId)
{
  // retriving the root of the given process id.
  int rootProcessId = getRootProcessId(providedProcessId, providedRootProcessId);
  // calling isBelongsToProcessTree  to check if it belongs to the process tree or not.
  if(isBelongsToProcessTree(rootProcessId, providedProcessId, providedRootProcessId))
  {
    // If belongs, then list all the process.
    listProcessId(providedProcessId, providedRootProcessId);
  }
  else
  {
    exitMethod();
  }
}

//Method used to pause a given process id.
void makeAProcessPause(long int providedProcessId)
{
	kill(providedProcessId, SIGSTOP) == 0 ? printf("Process %d paused with SIGSTOP\n", providedProcessId) : displayMessageToUser("Error occured while sending the SIGSTOP signal.");
}

// Method to perform differnt operation based on the options provided by the user.
void validateOption(char *providedOptionValue, int providedProcessId, int providedRootProcessId)
{
   if(strcmp(providedOptionValue, "-dx") == 0)
   {
      sendSIGSTOPOrSIGKILLSignal(providedRootProcessId, 0);
   }
   else if(strcmp(providedOptionValue, "-dt") == 0)
   {
      sendSIGSTOPOrSIGKILLSignal(providedRootProcessId, 1);
   }
   else if(strcmp(providedOptionValue, "-dc") == 0)
   {
      sendSIGCONTSignal(providedRootProcessId);
   }
   else if(strcmp(providedOptionValue, "-rp") == 0)
   {
      callToKillAProcess(providedProcessId);
   }
   else if(strcmp(providedOptionValue, "-nd") == 0)
   {
    getNonImmediateDescendants(providedProcessId, GETNONDIRECT);
    displayListofSiblings(allNonDirectChild, "There is no non-direct descendants.", 0);

   }
   else if(strcmp(providedOptionValue, "-dd") == 0)
   {
    getChildren(providedProcessId, GETDIRECT);
    displayListofSiblings(allDirectChild, "There is no immediate descendants.", 0);
   }
   else if(strcmp(providedOptionValue, "-sb") == 0)
   {
      prvProcessId = providedProcessId;
      retriveAllSiblingProcess(providedProcessId);
   }
   else if(strcmp(providedOptionValue, "-bz") == 0)
   {
      displayDefunctSibling(providedProcessId);
   }
   else if(strcmp(providedOptionValue, "-zd") == 0)
   {
      killParentOrListZombieProcess(providedProcessId, 1);
   }
   else if(strcmp(providedOptionValue, "-gc") == 0)
   {
      getNonImmediateDescendants(providedProcessId, GETGRANDCHILDREN);
      displayListofSiblings(allGrandChild, "There is no grand children for the given Process Id.", 0);
   }
   else if(strcmp(providedOptionValue, "-sz") == 0)
   {
      int isDefunctProcess = getProcessStatus(providedProcessId, 1);
      if(isDefunctProcess) 
        printf("The given Process Id %d is defunct.\n", providedProcessId);
      else 
        printf("The given Process Id %d is not defunct.\n", providedProcessId);
   }
   else if(strcmp(providedOptionValue, "-kz") == 0)
   {
     killParentOrListZombieProcess(providedProcessId, 0);
   }
   else
   {
      displayMessageToUser("Please provide a correct option.");
      exitMethod();
   }
}

//Main method.
int main(int argc, char *argv[])
{
  // Below three Variables used to store the user input like Option, Root and process Id.
  char *providedOptionValue = "";
  int providedRootProcessId = -1;
  int providedProcessId = -1;
  // Calling to check if the provided input is in a given format or not.
  validateInput(argc, argv);

// If has all the 3 values, the extracting the option and storing it in providedOptionValue.
  if(argc == 4)
  {
    providedOptionValue = argv[1];
  }

// now extracting the Root process Id and the process Id based on the user input.
  char *getRootPID = (argc == 4) ? argv[2] : argv[1];
  char *getPID = (argc == 4) ? argv[3] : argv[2];

  // Calling setInputVariable to convert the string input into integer input.
  providedRootProcessId = setInputVariable(getRootPID);
  providedProcessId = setInputVariable(getPID);

  // Calling the method to check if the provided input is a valid process Id or not same for root id.
  validateProcessId(providedRootProcessId, 1);
  validateProcessId(providedProcessId, 0);
 
  //Argument is 3 that is the user did not give the option
  if(argc == 3)
  {
    // below 2 are the global variables, and it is being used to print the message about a given process id belongs to process tree or not.
    inputProcessId = providedProcessId;
    inputRootProcessId = providedRootProcessId;
    validateInputWithNoOption = 1;
    // If not option provided, then calling this method to list the process tree of the given process id, if possible.
    performActionIfNoOptionProveided(providedProcessId, providedRootProcessId);
    validateInputWithNoOption = 0;
  }
  else
  {
    // below 2 are the global variables, and it is being used to print the message about a given process id belongs to process tree or not.
    inputProcessId = providedProcessId;
    inputRootProcessId = providedRootProcessId;

    // If option provided, then first validating that the given process Id belongs to process tree or not.
    //And then I am executing the relevant operation for the given option.
     int rootProcessId = getRootProcessId(providedProcessId, providedRootProcessId);
     if(isBelongsToProcessTree(rootProcessId, providedProcessId, providedRootProcessId))
     {
        validateOption(providedOptionValue,  providedProcessId, providedRootProcessId);
     }
     else
     {
      exitMethod();
     }
  }

 }


