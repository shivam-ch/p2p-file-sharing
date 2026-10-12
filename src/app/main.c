#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include "../transfer/transfer.h"
#include "../network/peer.h"
#include <sys/stat.h>
#include "p2p.h"
#include "../storage/file_splitter.h"
#include "../storage/piece_scheduler.h"
#define MAX_CONCURRENT_DOWNLOADS 4

static int parse_file_path(
    const char *command,
    const char *keyword,
    char *path,
    size_t path_size)
{
    size_t keyword_len = strlen(keyword);

    if (strncmp(command, keyword, keyword_len) != 0 ||
        command[keyword_len] != ' ')
    {
        return 0;
    }

    const char *p = command + keyword_len;

    while (*p == ' ')
    {
        p++;
    }

    if (*p == '\0' || *p == '\n')
    {
        return -1;
    }

    size_t len = 0;

    if (*p == '"')
    {
        p++;

        while (*p != '\0' && *p != '"' && *p != '\n')
        {
            if (len + 1 >= path_size)
            {
                return -1;
            }

            path[len++] = *p++;
        }

        if (*p != '"')
        {
            return -1;
        }

        p++;
    }
    else
    {
        while (*p != '\0' && *p != '\n' && *p != ' ' &&
               *p != '\t')
        {
            if (len + 1 >= path_size)
            {
                return -1;
            }

            path[len++] = *p++;
        }
    }

    path[len] = '\0';

    while (*p == ' ' || *p == '\t')
    {
        p++;
    }

    if (len == 0 || (*p != '\0' && *p != '\n'))
    {
        return -1;
    }

    return 1;
}

static void print_help(void)
{
    printf("\n");
    printf("========== P2P FILE SHARING COMMANDS ==========\n");
    printf("help / ?                 Show available commands\n");
    printf("status                   Show local node information\n");
    printf("connect <ip> <port>      Connect to another peer\n");
    printf("peers                    List known peers\n");
    printf("share <file_path>        Split and share a local file\n");
    printf("download <output_path>   Download and reconstruct a file\n");
    printf("fairness                 Show peer contribution statistics\n");
    printf("exit                     Save state and shut down\n");
    printf("===============================================\n");

    printf("\nExamples:\n");
    printf("  connect 127.0.0.1 5002\n");
    printf("  share ./files/document.pdf\n");
    printf("  download ./downloads/document.pdf\n");

    printf("\nNotes:\n");
    printf("  - Connect to a peer before downloading its shared file.\n");
    printf("  - Downloads use concurrent piece transfers and verification.\n");
    printf("  - Use quotes for file paths containing spaces.\n\n");
}

typedef struct
{
    AvailabilityTable *availability;
    pthread_mutex_t *availability_mutex;
    PeerTable *peers;
    pthread_mutex_t *peers_mutex;
    TransferPiece *piece;
    const char *pieces_dir;
    int result;
    uint32_t requester_peer_id;
    FairnessTable *fairness;
    pthread_mutex_t *fairness_mutex;
    const char *fairness_path;
} PeerDownloadJob;

static void *peer_download_worker(void *arg)
{
    PeerDownloadJob *job = arg;

    job->result = download_piece_from_peer_tracked(
        job->availability,
        job->availability_mutex,
        job->peers,
        job->peers_mutex,
        job->piece->info.piece_id,
        job->piece,
        job->pieces_dir,
        job->requester_peer_id,
        job->fairness,
        job->fairness_mutex);
    if (job->result == 0 &&
        job->fairness != NULL &&
        job->fairness_mutex != NULL &&
        job->fairness_path != NULL)
    {

        pthread_mutex_lock(job->fairness_mutex);

        int save_result = fairness_save(
            job->fairness,
            job->fairness_path);

        pthread_mutex_unlock(job->fairness_mutex);

        if (save_result != 0)
        {
            fprintf(stderr, "Warning: failed to save download fairness history.\n");
        }
    }
    return NULL;
}

static int parse_positive_number(
    const char *text,
    unsigned long max_value,
    unsigned long *result)
{
    char *end;
    unsigned long value;

    if (text == NULL || *text == '\0' || *text == '-')
    {
        return -1;
    }

    errno = 0;
    value = strtoul(text, &end, 10);

    if (errno == ERANGE ||
        end == text ||
        *end != '\0' ||
        value == 0 ||
        value > max_value)
    {
        return -1;
    }

    *result = value;
    return 0;
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <peer_id> <port>\n", argv[0]);
        return 1;
    }

    unsigned long parsed_peer_id;
    unsigned long parsed_port;

    if (parse_positive_number(argv[1], UINT32_MAX,
                              &parsed_peer_id) != 0)
    {
        fprintf(stderr, "Error: peer ID must be a positive integer.\n");
        return 1;
    }

    if (parse_positive_number(argv[2], 65535,
                              &parsed_port) != 0)
    {
        fprintf(stderr, "Error: port must be an integer from 1 to 65535.\n");
        return 1;
    }

    uint32_t peer_id = (uint32_t)parsed_peer_id;
    uint16_t port = (uint16_t)parsed_port;

    P2PNode node;

    if (p2p_node_init(&node, peer_id, port) != 0)
    {
        printf("Failed to initialize P2P node.\n");
        return 1;
    }
    char fairness_path[512];

    int fairness_path_len = snprintf(
        fairness_path,
        sizeof(fairness_path),
        "%s/fairness.csv",
        node.pieces_dir);

    if (fairness_path_len < 0 ||
        (size_t)fairness_path_len >= sizeof(fairness_path))
    {
        printf("Fairness file path is too long.\n");
        p2p_node_free(&node);
        return 1;
    }

    if (fairness_load(&node.fairness, fairness_path) == 0)
    {
        printf("Loaded saved fairness history.\n");
    }
    else
    {
        printf("No saved fairness history found; starting fresh.\n");
    }

    if (p2p_start_listener(&node) != 0)
    {
        printf("Failed to start listener on port %u.\n", port);
        p2p_node_free(&node);
        return 1;
    }

    printf("\n");
    printf("================================================\n");
    printf("          P2P FILE SHARING NETWORK              \n");
    printf("================================================\n");
    printf("  Status          : ONLINE\n");
    printf("  Peer ID         : %u\n", peer_id);
    printf("  Listening port  : %u\n", port);
    printf("  Storage folder  : %s\n", node.pieces_dir);
    printf("================================================\n");
    printf("Type 'help' to see available commands.\n");
    printf("================================================\n\n");

    char command[256];

    while (node.running)
    {

        printf("p2p> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            break;
        }

        if (strcmp(command, "exit\n") == 0 ||
            strcmp(command, "exit") == 0)
        {
            break;
        }
        if (strcmp(command, "help\n") == 0 ||
            strcmp(command, "help") == 0 ||
            strcmp(command, "?\n") == 0 ||
            strcmp(command, "?") == 0)
        {
            print_help();
            continue;
        }
        if (strcmp(command, "status\n") == 0 ||
            strcmp(command, "status") == 0)
        {
            printf("\n========== NODE STATUS ==========\n");
            printf("Node state        : %s\n",
                   node.running ? "ONLINE" : "OFFLINE");
            printf("Peer ID           : %u\n", node.peer_id);
            printf("Listening port    : %u\n", node.port);
            printf("Storage directory : %s\n", node.pieces_dir);

            pthread_mutex_lock(&node.peers_mutex);
            printf("Known peers       : %zu\n", node.peers.count);
            pthread_mutex_unlock(&node.peers_mutex);

            pthread_mutex_lock(&node.availability_mutex);
            printf("Remote pieces     : %zu\n", node.remote_piece_count);
            pthread_mutex_unlock(&node.availability_mutex);

            printf("Shared file       : %s\n",
                   node.shared_file_path[0] != '\0'
                       ? node.shared_file_path
                       : "None");

            printf("Local pieces      : %zu\n", node.piece_count);

            pthread_mutex_lock(&node.fairness_mutex);
            printf("Tracked peers     : %zu\n",
                   node.fairness.peer_count);
            pthread_mutex_unlock(&node.fairness_mutex);

            printf("=================================\n\n");
            continue;
        }

        char ip[46];
        unsigned int port;

        char extra;

        if (sscanf(command, "connect %45s %u %c", ip, &port, &extra) == 2)
        {

            if (port == 0 || port > 65535)
            {
                printf("Invalid port. Use a port between 1 and 65535.\n");
                continue;
            }

            if (p2p_connect_to_peer(
                    &node,
                    ip,
                    (uint16_t)port) != 0)
            {

                printf("Failed to connect to peer.\n");
            }

            continue;
        }

        if (strcmp(command, "peers\n") == 0 ||
            strcmp(command, "peers") == 0)
        {
            pthread_mutex_lock(&node.peers_mutex);

            printf("\n");
            printf("=============== KNOWN PEERS ===============\n");
            printf("%-12s %-20s %-8s\n",
                   "Peer ID", "IP Address", "Port");
            printf("-------------------------------------------\n");

            if (node.peers.count == 0)
            {
                printf("No peers discovered yet.\n");
            }
            else
            {
                for (size_t i = 0; i < node.peers.count; i++)
                {
                    printf("%-12u %-20s %-8u\n",
                           node.peers.peers[i].peer_id,
                           node.peers.peers[i].ip,
                           node.peers.peers[i].port);
                }
            }

            printf("-------------------------------------------\n");
            printf("Total known peers: %zu\n", node.peers.count);
            printf("===========================================\n\n");

            pthread_mutex_unlock(&node.peers_mutex);
            continue;
        }

        if (strcmp(command, "fairness\n") == 0 ||
            strcmp(command, "fairness") == 0)
        {

            pthread_mutex_lock(&node.fairness_mutex);

            printf("\n========== PEER FAIRNESS REPORT ==========\n");
            printf("%-10s %-18s %-20s %-10s\n",
                   "Peer ID",
                   "Uploaded Pieces",
                   "Downloaded Pieces",
                   "Ratio");
            printf("------------------------------------------------------------\n");

            if (node.fairness.peer_count == 0)
            {
                printf("No peer contributions recorded yet.\n");
            }
            else
            {
                for (size_t i = 0;
                     i < node.fairness.peer_count;
                     i++)
                {

                    PeerContribution *peer =
                        &node.fairness.peers[i];

                    if (!peer->active)
                    {
                        continue;
                    }

                    printf("%-10u %-18u %-20u %.2f\n",
                           peer->peer_id,
                           peer->uploaded_pieces,
                           peer->downloaded_pieces,
                           fairness_get_ratio(peer));
                }
            }

            printf("============================================\n\n");

            pthread_mutex_unlock(&node.fairness_mutex);
            continue;
        }

        char file_path[256];

        int share_result = parse_file_path(
            command, "share", file_path, sizeof(file_path));

        if (share_result == 1)
        {
            PieceInfo *new_pieces = NULL;
            size_t new_count = 0;

            if (split_file(
                    file_path,
                    node.pieces_dir,
                    DEFAULT_PIECE_SIZE,
                    &new_pieces,
                    &new_count) != 0)
            {
                printf("Failed to split file.\n");
                free_pieces(new_pieces);
                continue;
            }

            free_pieces(node.pieces);
            node.pieces = new_pieces;
            node.piece_count = new_count;

            snprintf(
                node.shared_file_path,
                sizeof(node.shared_file_path),
                "%s",
                file_path);

            printf("File shared locally: %s\n", file_path);
            printf("Total pieces: %zu\n", node.piece_count);

            for (size_t i = 0; i < node.piece_count; i++)
            {
                printf(
                    "Piece %u | Size: %llu bytes\n",
                    node.pieces[i].piece_id,
                    (unsigned long long)node.pieces[i].size);
            }
            if (p2p_announce_manifest(&node) != 0)
            {
                printf("Warning: Some peers could not be notified.\n");
            }

            continue;
        }
        if (strcmp(command, "share\n") == 0 ||
            strcmp(command, "share") == 0)
        {
            printf("Missing file path. Example: share \"my documents/report.pdf\"\n");
            continue;
        }
        if (share_result == -1)
        {
            printf("Invalid share path. Use: share <path> or share \"<path with spaces>\"\n");
            continue;
        }

        char output_path[256];

        int download_result = parse_file_path(
            command, "download", output_path, sizeof(output_path));
        if (strcmp(command, "download\n") == 0 ||
            strcmp(command, "download") == 0)
        {
            printf("Missing output path. Example: download \"my downloads/report.pdf\"\n");
            continue;
        }
        if (download_result == 1)
        {
            pthread_mutex_lock(&node.availability_mutex);

            size_t count = node.remote_piece_count;
            PieceInfo *manifest_copy = NULL;

            if (node.remote_pieces != NULL && count > 0)
            {
                manifest_copy = malloc(count * sizeof(*manifest_copy));

                if (manifest_copy != NULL)
                {
                    memcpy(
                        manifest_copy,
                        node.remote_pieces,
                        count * sizeof(*manifest_copy));
                }
            }

            pthread_mutex_unlock(&node.availability_mutex);

            if (count == 0)
            {
                printf("No file manifest available.\n");
                printf("Connect to a peer that has shared a file, then retry.\n");
                free(manifest_copy);
                continue;
            }

            if (manifest_copy == NULL)
            {
                printf("Error: Could not allocate memory for the file manifest.\n");
                continue;
            }

            char download_dir[512];

            int written = snprintf(
                download_dir,
                sizeof(download_dir),
                "%s.pieces",
                output_path);

            if (written < 0 ||
                (size_t)written >= sizeof(download_dir))
            {
                printf("Output path is too long.\n");
                free(manifest_copy);
                continue;
            }

            int failed = 0;

            TransferPiece *pieces = calloc(count, sizeof(*pieces));
            PeerDownloadJob *jobs = calloc(count, sizeof(*jobs));
            size_t *piece_order = malloc(count * sizeof(*piece_order));

            if (pieces == NULL || jobs == NULL || piece_order == NULL)
            {
                printf("Failed to allocate download tasks.\n");
                free(pieces);
                free(jobs);
                free(piece_order);
                free(manifest_copy);
                continue;
            }
            pthread_mutex_lock(&node.availability_mutex);

            int sort_result = piece_scheduler_sort_by_rarity(
                piece_order,
                count,
                manifest_copy,
                &node.availability);

            pthread_mutex_unlock(&node.availability_mutex);

            if (sort_result != 0)
            {
                printf("Failed to sort pieces by rarity.\n");
                free(pieces);
                free(jobs);
                free(piece_order);
                free(manifest_copy);
                continue;
            }

            size_t initialized = 0;
            size_t completed_pieces = 0;

            for (size_t i = 0; i < count; i++)
            {
                pieces[i].info = manifest_copy[piece_order[i]];
                pieces[i].state = TRANSFER_MISSING;
                jobs[i].availability = &node.availability;
                jobs[i].availability_mutex = &node.availability_mutex;
                jobs[i].peers = &node.peers;
                jobs[i].peers_mutex = &node.peers_mutex;
                jobs[i].piece = &pieces[i];
                jobs[i].pieces_dir = node.pieces_dir;
                jobs[i].requester_peer_id = node.peer_id;
                jobs[i].result = -1;
                jobs[i].fairness = &node.fairness;
                jobs[i].fairness_mutex = &node.fairness_mutex;
                jobs[i].fairness_path = fairness_path;

                if (pthread_mutex_init(&pieces[i].mutex, NULL) != 0)
                {
                    printf("Failed to initialize piece mutex.\n");
                    failed = 1;
                    break;
                }

                initialized++;
            }
            free(piece_order);

            for (size_t start = 0;
                 !failed && start < count;
                 start += MAX_CONCURRENT_DOWNLOADS)
            {

                size_t batch = count - start;

                if (batch > MAX_CONCURRENT_DOWNLOADS)
                {
                    batch = MAX_CONCURRENT_DOWNLOADS;
                }

                pthread_t threads[MAX_CONCURRENT_DOWNLOADS];
                size_t started = 0;

                printf(
                    "Downloading pieces %zu to %zu concurrently...\n",
                    start,
                    start + batch - 1);

                for (size_t j = 0; j < batch; j++)
                {
                    if (pthread_create(
                            &threads[j],
                            NULL,
                            peer_download_worker,
                            &jobs[start + j]) != 0)
                    {
                        printf("Failed to start download thread.\n");
                        failed = 1;
                        break;
                    }

                    started++;
                }

                for (size_t j = 0; j < started; j++)
                {
                    pthread_join(threads[j], NULL);
                }

                if (started != batch)
                {
                    failed = 1;
                }

                for (size_t j = 0; j < started; j++)
                {
                    size_t index = start + j;

                    if (jobs[index].result != 0)
                    {
                        printf(
                            "ERROR: Piece %u could not be downloaded or verified.\n",
                            pieces[index].info.piece_id);
                        failed = 1;
                    }
                    else
                    {
                        completed_pieces++;

                        printf(
                            "Piece %u downloaded and verified.\n",
                            pieces[index].info.piece_id);
                    }
                }

                double progress = (double)completed_pieces * 100.0 /
                                  (double)count;

                printf(
                    "Download progress: %zu/%zu pieces (%.1f%%)\n\n",
                    completed_pieces,
                    count,
                    progress);
            }

            for (size_t i = 0; i < initialized; i++)
            {
                pthread_mutex_destroy(&pieces[i].mutex);
            }

            free(jobs);
            free(pieces);

            if (failed)
            {
                printf("Download incomplete: %zu of %zu pieces verified.\n",
                       completed_pieces, count);
                printf("Check peer connectivity and availability, then retry.\n");
                free(manifest_copy);
                continue;
            }

            if (reconstruct_file(
                    output_path,
                    node.pieces_dir,
                    manifest_copy,
                    count) != 0)
            {
                printf("ERROR: Downloaded pieces are available, but reconstruction failed.\n");
                printf("Check the output path, piece files, and manifest order.\n");
                free(manifest_copy);
                continue;
            }

            printf("\n");
            printf("========================================\n");
            printf("          DOWNLOAD SUCCESSFUL           \n");
            printf("========================================\n");
            printf("File       : %s\n", output_path);
            printf("Pieces     : %zu/%zu verified\n",
                   completed_pieces, count);
            printf("Progress   : 100%%\n");
            printf("========================================\n\n");
            free(manifest_copy);
            continue;
        }
        if (download_result == -1)
        {
            printf("Invalid download path. Use: download <path> or download \"<path with spaces>\"\n");
            continue;
        }

        printf("Unknown command. Type 'help' to see available commands.\n");
    }

    pthread_mutex_lock(&node.fairness_mutex);

    if (fairness_save(&node.fairness, fairness_path) == 0)
    {
        printf("Fairness history saved to %s\n", fairness_path);
    }
    else
    {
        printf("Warning: failed to save fairness history.\n");
    }

    pthread_mutex_unlock(&node.fairness_mutex);

    p2p_node_free(&node);

    return 0;
}
