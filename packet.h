#define PACKET_SIZE 32

// Send: DataPacket
// Recieve: FilePacket
#define MSG_GET_FILE 1

// Send: DataPacket
#define MSG_CREATE_FILE 2

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
  char unused[18];
  int arg1; /* integer argument (4 bytes) */
  void *pointer; /* pointer argument (8 bytes) */
} DataPacket;

typedef struct FilePacket {
  short packet_type; /* packet type (2 bytes) */
  char unused[14]; /* 14 unused bytes for padding */

  int inum; /* inode number (4 bytes) */
  int type; /* type of file (4 bytes) */
  int size; /* size of file in bytes (4 bytes) */
  int nlink; /* link count of file's inode (4 bytes) */
} FilePacket;
