#define PACKET_SIZE 32

// Send: FilePacket
// Recieve: FilePacket
#define MSG_GET_FILE 0

// Send: DataPacket
// Recieve: FilePacket
#define MSG_SEARCH_FILE 1

// Send: DataPacket
// Recieve: FilePacket
#define MSG_CREATE_FILE 2

// Send: DataPacket
// Recieve: DataPacket
#define MSG_READ_FILE 3

// Send: DataPacket
// Recieve: DataPacket
#define MSG_WRITE_FILE 4

// Send: DataPacket
// Recieve: FilePacket
#define MSG_CREATE_DIR 5

// Send: DataPacket
// Receive: DataPacket
#define MSG_DELETE_DIR 6

#define MSG_LINK 7

#define MSG_UNLINK 8

// Send: DataPacket
// Receive DataPacket
#define MSG_SYNC 9

/*
 * All of the below must have size of 32 bytes.
 */

/*
 * Unknown packet
 * Must be used by file server just for identifying packet_type.
 */
typedef struct UnknownPacket {
  short packet_type;
  char name[30];
} UnknownPacket;

/*
 * Packet for multiple purpose
 */
typedef struct DataPacket {
  short packet_type; /* packet type (2 bytes) */
  char unused[6];
  int arg1; /* integer argument (4 bytes) */
  int arg2; /* integer argument (4 bytes) */
  int arg3; /* integer argument (4 bytes) */
  int arg4; /* integer arugment (4 bytes) */
  void *pointer; /* pointer argument (8 bytes) */
} DataPacket;

/*
 * Packet for returning file data
 */
typedef struct FilePacket {
  short packet_type; /* packet type (2 bytes) */
  char unused[10]; /* 10 unused bytes for padding */

  int inum; /* inode number (4 bytes) */
  int type; /* type of file (4 bytes) */
  int size; /* size of file in bytes (4 bytes) */
  int nlink; /* link count of file's inode (4 bytes) */
  int reuse; /* reuse count of file's inode (4 bytes )*/
} FilePacket;
