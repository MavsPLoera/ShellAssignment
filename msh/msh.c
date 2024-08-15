// The MIT License (MIT)
// 
// Copyright (c) 2024 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 32     

int main(int argc, char * argv[])
{
  char error_message[30] = "An error has occurred\n";
  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );
  FILE *fp;

  if(argc > 2) //More than one argument
  {
    write(STDERR_FILENO, error_message, strlen(error_message));
    return 1;
  }
  else if(argc == 2) // Valid amount of arguments for "batchmode"
  {
    fp = fopen(argv[1], "rb");

    if(fp == NULL) //Invalid file
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }
    else //Valid file
    {
        while(fgets(command_string, MAX_COMMAND_SIZE, fp) != NULL) //While loop that will run until the EOF
        {
            char *tokens[MAX_NUM_ARGUMENTS];
            int num_string_input = 0;                                 
            char *argument_pointer;                                                                                                  
            char *working_string  = strdup( command_string );                
            char *head_ptr = working_string;
            
            while ( ( (argument_pointer = strsep(&working_string, WHITESPACE ) ) != NULL) &&
                    (num_string_input<MAX_NUM_ARGUMENTS))
            {
                tokens[num_string_input] = strndup( argument_pointer, MAX_COMMAND_SIZE );
                if( strlen( tokens[num_string_input] ) == 0 )
                {
                    tokens[num_string_input] = NULL;
                }
                num_string_input++;
            }

            int token_index = 0;
            pid_t child_pid;
            int status;

            //Take inputed string and check for variable white space. If input at i position is not NULL add it to a new char* array with no whitespace
            char *token[MAX_NUM_ARGUMENTS];
            int token_count = 0;
            int index = 0;

            for(int i = 0; i < num_string_input-1; i++)
            {
                if(tokens[i] != NULL)
                {
                    token[index] = tokens[i];
                    token_count++;
                    index++;
                }
            }
            token[index] = '\0'; //add nullbyte for new line character
            token_count++;

            if(!(token[token_index] == NULL) && (strlen(token[token_index]) != 1 || strcmp(token[token_index],">") == 0)) //Checks if token[token_index] is not equal to NULL or a single character that is not ">".
            {
                if((strcmp(token[token_index],"exit") == 0)) //Exit program cleanly if only "exit" is typed
                {
                    if((token_count-1) < 2)
                    {
                        exit(EXIT_SUCCESS);
                    }
                    else
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    }
                }
                else if(strcmp(token[token_index],"cd") == 0) //Check if user asked to change directory
                {
                    if(token[token_index+1] == NULL)
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message)); 
                    }
                    else if((token_count-1) > 2) //More than one argument with cd
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    }
                    else
                    {
                        int status = chdir(token[token_index+1]);
                        if(status < 0)
                        {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        } 
                    }
                }
                else
                {
                    child_pid = fork();

                    if( child_pid == 0 )
                    {
                        if(strcmp(token[token_index],">") == 0) //Checks first token to see if equal to ">"
                        {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        }
                        else
                        {
                            for(int i = token_index; i < token_count-1; i++) //For loop for checking for redirection inside each token besides tokens equal to ">"
                            {
                                //Could possibly add a shift to the tokens to make this more realistic. Instead of hard coding the values that could affect other tokens.
                                if(strchr( token[i], '>' ) != NULL && strlen(token[i]) != 1)
                                {
                                    char* temp = token[i];
                                    token[i] = strtok(temp,">");
                                    token[i+1] = ">";
                                    token[i+2] = strtok(NULL,">");
                                    token[i+3] = NULL;
                                    token_count += 2;
                                }
                            }

                            //Check for multiple redirection
                            int redirectCount = 0;
                            int redirectLocation = 0;
                            for(int i = token_index; i < token_count-1; i++)
                            {
                                if( strcmp( token[i], ">" ) == 0 )
                                {
                                    redirectCount++;
                                    redirectLocation = i;
                                }
                            }


                            if(redirectCount > 1) //More than one redirection found
                            {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                            }
                            else if(redirectCount == 0) //No redirection just normal command
                            {
                                int ret = execvp(token[token_index], &token[token_index]);  
                                if( ret == -1 )
                                {
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                }
                            }
                            else //redirection = 1
                            {
                                if(((token_count-1)-redirectLocation) != 2) //Check if arguments after ">" is not > 1 and not < 1 (JUST EQUAL TO ONE)
                                {
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                }
                                else
                                {
                                    for(int i = token_index; i < token_count-1; i++ ) //Prepare token for execvp with redirection
                                    {
                                        if( strcmp( token[i], ">" ) == 0 )
                                        {
                                            int fd = open( token[i+1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR );
                                            if( fd < 0 )
                                            {
                                                perror( "Can't open output file." );
                                                exit( 0 );                    
                                            }
                                            dup2( fd, 1 );
                                            close( fd );
                                            
                                            // Trim off the > output part of the command
                                            token[i] = NULL;
                                            break;
                                        }
                                    }

                                    int ret = execvp(token[token_index], &token[token_index]);  
                                    if( ret == -1 )
                                    {
                                        write(STDERR_FILENO, error_message, strlen(error_message));
                                    }
                                }

                            }
                        }
                    }
                    waitpid(child_pid, &status, 0);
                }

                //Need to reset the token array since it is not taking in new user input directly
                for(int i = 0; i < token_count-1; i++)
                {
                    token[i] = NULL;
                }


                free( head_ptr );
            }
        }
        fclose(fp);
    
    }
  }
  else
  {
    while( 1 )
    {
        printf ("msh> ");

        //Get stdin from user and put the input into a char* called tokens
        while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );
        char *tokens[MAX_NUM_ARGUMENTS];
        int num_string_input = 0;                                 
        char *argument_pointer;                                                                                                  
        char *working_string  = strdup( command_string );                
        char *head_ptr = working_string;
        
        while ( ( (argument_pointer = strsep(&working_string, WHITESPACE ) ) != NULL) &&
                (num_string_input<MAX_NUM_ARGUMENTS))
        {
            tokens[num_string_input] = strndup( argument_pointer, MAX_COMMAND_SIZE );
            if( strlen( tokens[num_string_input] ) == 0 )
            {
                tokens[num_string_input] = NULL;
            }
            num_string_input++;
        }

        int token_index = 0;
        pid_t child_pid;
        int status;

        //Take inputed string and check for variable white space. If input at i position is not NULL add it to a new char* array with no whitespace
        char *token[MAX_NUM_ARGUMENTS];
        int token_count = 0;
        int index = 0;

        for(int i = 0; i < num_string_input-1; i++)
        {
            if(tokens[i] != NULL)
            {
                token[index] = tokens[i];
                token_count++;
                index++;
            }
        }
        token[index] = '\0'; //add nullbyte for new line character
        token_count++;

        if(!(token[token_index] == NULL) && (strlen(token[token_index]) != 1 || strcmp(token[token_index],">") == 0)) //Checks if token[token_index] is not equal to NULL or a single character that is not ">".
        {
            if((strcmp(token[token_index],"exit") == 0)) //Exit program cleanly if only "exit" is typed
            {
                if((token_count-1) < 2)
                {
                    exit(EXIT_SUCCESS);
                }
                else
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
            else if(strcmp(token[token_index],"cd") == 0) //Check if user asked to change directory
            {
                if(token[token_index+1] == NULL)
                {
                    write(STDERR_FILENO, error_message, strlen(error_message)); 
                }
                else if((token_count-1) > 2) //More than one argument with cd
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else
                {
                    int status = chdir(token[token_index+1]);
                    if(status < 0)
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    } 
                }
            }
            else
            {
                child_pid = fork();

                if( child_pid == 0 )
                {
                    if(strcmp(token[token_index],">") == 0) //Checks first token to see if equal to ">"
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    }
                    else
                    {
                        for(int i = token_index; i < token_count-1; i++) //For loop for checking for redirection inside each token besides tokens equal to ">"
                        {
                            //Could possibly add a shift to the tokens to make this more realistic. Instead of hard coding the values that could affect other tokens.
                            if(strchr( token[i], '>' ) != NULL && strlen(token[i]) != 1)
                            {
                                char* temp = token[i];
                                token[i] = strtok(temp,">");
                                token[i+1] = ">";
                                token[i+2] = strtok(NULL,">");
                                token[i+3] = NULL;
                                token_count += 2;
                            }
                        }

                        //Check for multiple redirection
                        int redirectCount = 0;
                        int redirectLocation = 0;
                        for(int i = token_index; i < token_count-1; i++)
                        {
                            if( strcmp( token[i], ">" ) == 0 )
                            {
                                redirectCount++;
                                redirectLocation = i;
                            }
                        }


                        if(redirectCount > 1) //More than one redirection found
                        {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        }
                        else if(redirectCount == 0) //No redirection just normal command
                        {
                            int ret = execvp(token[token_index], &token[token_index]);  
                            if( ret == -1 )
                            {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                            }
                        }
                        else //redirection = 1
                        {
                            if(((token_count-1)-redirectLocation) != 2) //Check if arguments after ">" is not > 1 and not < 1 (JUST EQUAL TO ONE)
                            {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                            }
                            else
                            {
                                for(int i = token_index; i < token_count-1; i++ ) //Prepare token for execvp with redirection
                                {
                                    if( strcmp( token[i], ">" ) == 0 )
                                    {
                                        int fd = open( token[i+1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR );
                                        if( fd < 0 )
                                        {
                                            perror( "Can't open output file." );
                                            exit( 0 );                    
                                        }
                                        dup2( fd, 1 );
                                        close( fd );
                                        
                                        // Trim off the > output part of the command
                                        token[i] = NULL;
                                        break;
                                    }
                                }

                                int ret = execvp(token[token_index], &token[token_index]);  
                                if( ret == -1 )
                                {
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                }
                            }

                        }
                    }
                }
                waitpid(child_pid, &status, 0);
            }

            //Need to reset the token array since it is not taking in new user input directly
            for(int i = 0; i < token_count-1; i++)
            {
                token[i] = NULL;
            }


            free( head_ptr );
        }
    }
  }

  return 0;
  // e2520ca2-76f3-90d6-0242ac1210022
}