#!/bin/bash

ARGS="--daemon"
# DCORE_P2P_ENDPOINT
# DCORE_SEED_NODES
# DCORE_IPFS_API
# DCORE_REPLAY_BLOCKCHAIN
# DCORE_RESYNC_BLOCKCHAIN

# account history plugin
# DCORE_TRACK_ACCOUNTS

# miner plugin
# DCORE_MINER_ID
# DCORE_PRIVATE_KEY

# seeding plugin
# DCORE_SEEDER
# DCORE_CONTENT_PRIVATE_KEY
# DCORE_SEEDER_PRIVATE_KEY
# DCORE_FREE_SPACE
# DCORE_SEEDING_PRICE
# DCORE_SEEDING_SYMBOL
# DCORE_REGION_CODE

# transaction history plugin
# DCORE_TRANSACTION_ID_HISTORY

if [[ ! -z "$DCORE_P2P_ENDPOINT" ]]; then
    ARGS+=" --p2p-endpoint=$DCORE_P2P_ENDPOINT"
fi

if [[ ! -z "$DCORE_SEED_NODES" ]]; then
    for SEED_NODE in $DCORE_SEED_NODES; do
        ARGS+=" --seed-node=$SEED_NODE"
    done
fi

if [[ ! -z "$DCORE_IPFS_API" ]]; then
    ARGS+=" --ipfs-api=$DCORE_IPFS_API"
fi

if [[ ! -z "$DCORE_REPLAY_BLOCKCHAIN" ]]; then
    ARGS+=" --replay-blockchain"
fi

if [[ ! -z "$DCORE_RESYNC_BLOCKCHAIN" ]]; then
    ARGS+=" --resync-blockchain"
fi

if [[ ! -z "$DCORE_TRACK_ACCOUNTS" ]]; then
    for TRACK_ACCOUNT in $DCORE_TRACK_ACCOUNTS; do
        ARGS+=" --track-account=$TRACK_ACCOUNT"
    done
fi

if [[ ! -z "$DCORE_MINER_ID" ]]; then
    ARGS+=" --miner-id=$DCORE_MINER_ID"
fi

if [[ ! -z "$DCORE_PRIVATE_KEY" ]]; then
    ARGS+=" --private-key=$DCORE_PRIVATE_KEY"
fi

if [[ ! -z "$DCORE_SEEDER" ]]; then
    ARGS+=" --seeder=$DCORE_SEEDER"
fi

if [[ ! -z "$DCORE_CONTENT_PRIVATE_KEY" ]]; then
    ARGS+=" --content-private-key=$DCORE_CONTENT_PRIVATE_KEY"
fi

if [[ ! -z "$DCORE_SEEDER_PRIVATE_KEY" ]]; then
    ARGS+=" --seeder-private-key=$DCORE_SEEDER_PRIVATE_KEY"
fi

if [[ ! -z "$DCORE_FREE_SPACE" ]]; then
    ARGS+=" --free-space=$DCORE_FREE_SPACE"
fi

if [[ ! -z "$DCORE_SEEDING_PRICE" ]]; then
    ARGS+=" --seeding-price=$DCORE_SEEDING_PRICE"
fi

if [[ ! -z "$DCORE_SEEDING_SYMBOL" ]]; then
    ARGS+=" --seeding-symbol=$DCORE_SEEDING_SYMBOL"
fi

if [[ ! -z "$DCORE_REGION_CODE" ]]; then
    ARGS+=" --region-code=$DCORE_REGION_CODE"
fi

if [[ ! -z "$DCORE_TRANSACTION_ID_HISTORY" ]]; then
    ARGS+=" --transaction-id-history=$DCORE_TRANSACTION_ID_HISTORY"
fi

decentd $ARGS $DCORE_EXTRA_ARGS

sleep infinity