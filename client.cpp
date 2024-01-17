#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345

// Structure representing a node in the tree
typedef struct TreeNode
{
    int key;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

// Structure for task data sent to the server
typedef struct
{
    TreeNode *treeRoot; // Root of the tree
    int limits;         // Limits (if needed)
} task_data_t;

// Function to create a sample tree (modify as needed)
TreeNode *createSampleTree()
{
    TreeNode *root = (TreeNode *)malloc(sizeof(TreeNode));
    root->key = 5;
    root->left = (TreeNode *)malloc(sizeof(TreeNode));
    root->left->key = 3;
    root->left->left = NULL;
    root->left->right = NULL;
    root->right = (TreeNode *)malloc(sizeof(TreeNode));
    root->right->key = 8;
    root->right->left = NULL;
    root->right->right = NULL;
    return root;
}

// Function to send task data to the server
void sendTask(int serverSocket, TreeNode *treeRoot)
{
    task_data_t data;
    data.treeRoot = treeRoot;
    data.limits = 10; // Modify as needed

    // Send task data to the server
    if (send(serverSocket, &data, sizeof(data), 0) < 0)
    {
        perror("Sending error");
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <server_ip>" << std::endl;
        return 1;
    }

    int clientSocket;
    struct sockaddr_in serverAddr;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        perror("Error creating socket");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &(serverAddr.sin_addr)) <= 0)
    {
        perror("Error converting IP address");
        close(clientSocket);
        return 1;
    }

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error connecting to server");
        close(clientSocket);
        return 1;
    }

    // Create a sample tree (modify as needed)
    TreeNode *sampleTree = createSampleTree();

    // Send the task to the server
    sendTask(clientSocket, sampleTree);

    // Receive the result from the server
    double result;
    if (recv(clientSocket, &result, sizeof(result), 0) < 0)
    {
        perror("Receiving error");
    }
    else
    {
        std::cout << "Received result from server: " << result << std::endl;
    }

    close(clientSocket);

    return 0;
}



