#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345

// Structure representing a node in the tree
typedef struct TreeNode
{
  int key;
  struct TreeNode *left;
  struct TreeNode *right;
} TreeNode;

// Function to serialize the tree into a file
void serializeTree(TreeNode *node, FILE *file)
{
  if (node == NULL)
  {
    fwrite(&node, sizeof(TreeNode *), 1, file);
    return;
  }

  fwrite(&node->key, sizeof(int), 1, file);
  serializeTree(node->left, file);
  serializeTree(node->right, file);
}

// Function to deserialize the tree from a file
TreeNode *deserializeTree(FILE *file)
{
  TreeNode *node;
  fread(&node, sizeof(TreeNode *), 1, file);

  if (node == NULL)
    return NULL;

  node = (TreeNode *)malloc(sizeof(TreeNode));

  fread(&node->key, sizeof(int), 1, file);
  node->left = deserializeTree(file);
  node->right = deserializeTree(file);

  return node;
}

// Structure for task data sent from the client
typedef struct
{
  TreeNode *treeRoot; // Root of the tree
  int limits;         // Limits (if needed)
} task_data_t;

// Function to traverse the tree and calculate the sum of all keys
double traverseTree(TreeNode *node)
{
  if (node == NULL)
    return 0.0;

  double leftSum = traverseTree(node->left);
  double rightSum = traverseTree(node->right);

  return node->key + leftSum + rightSum;
}

// Function to handle the client's request and send back the result
void *handleClient(void *arg)
{
  int clientSocket = *(int *)arg;

  task_data_t data;

  // Receive data from the client
  int read_bytes = recv(clientSocket, &data, sizeof(data), 0);
  if (read_bytes != sizeof(data))
  {
    std::cerr << "Invalid data from client" << std::endl;
    close(clientSocket);
    pthread_exit(NULL);
  }

  std::cout << "Received task from client" << std::endl;

  // Calculate the sum of all keys in the tree
  double result = traverseTree(data.treeRoot);

  // Send the result back to the client
  if (send(clientSocket, &result, sizeof(result), 0) < 0)
  {
    perror("Sending error");
  }

  close(clientSocket);

  pthread_exit(NULL);
}

// Function to send the sample tree to the client
void *send_thread(void *arg)
{
  int serverSocket = *(int *)arg;

  int clientSocket = accept(serverSocket, NULL, NULL);
  if (clientSocket < 0)
  {
    perror("Error accepting client connection");
    close(serverSocket);
    pthread_exit(NULL);
  }

  FILE *treeFile = fopen("tree.dat", "rb");
  if (treeFile)
  {
    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), treeFile)) > 0)
    {
      send(clientSocket, buffer, bytesRead, 0);
    }
    fclose(treeFile);
  }

  close(clientSocket);

  pthread_exit(NULL);
}

int main()
{
  int serverSocket;
  struct sockaddr_in serverAddr;

  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket < 0)
  {
    perror("Error creating socket");
    return 1;
  }

  FILE *treeFile = fopen("tree.dat", "rb");
  TreeNode *sampleTree;
  if (treeFile)
  {
    sampleTree = deserializeTree(treeFile);
    fclose(treeFile);
  }
  else
  {
    std::cerr << "Tree file not found. Create a sample tree first." << std::endl;
    close(serverSocket);
    return 1;
  }

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(PORT);
  serverAddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
  {
    perror("Error binding socket");
    close(serverSocket);
    return 1;
  }

  if (listen(serverSocket, 5) < 0)
  {
    perror("Error listening on socket");
    close(serverSocket);
    return 1;
  }

  std::cout << "Server listening on port " << PORT << "..." << std::endl;

  // Create a thread to send the sample tree to the client
  pthread_t sendThread;
  if (pthread_create(&sendThread, NULL, send_thread, &serverSocket) != 0)
  {
    perror("Error creating send_thread");
    close(serverSocket);
    return 1;
  }

  // Detach the thread to avoid memory leaks
  pthread_detach(sendThread);

  while (1)
  {
    int clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket < 0)
    {
      perror("Error accepting client connection");
      continue;
    }

    // Create a thread to handle the client's request
    pthread_t clientThread;
    if (pthread_create(&clientThread, NULL, handleClient, (void *)&clientSocket) != 0)
    {
      perror("Error creating client thread");
      close(clientSocket);
    }
    else
    {
      // Detach the thread to avoid memory leaks
      pthread_detach(clientThread);
    }
  }

  close(serverSocket);

  return 0;
}

