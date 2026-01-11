-- Migration: Create devices table for linking firmware devices to user accounts
-- Run this in your Supabase SQL Editor

-- Devices table: Maps Tuya device IDs to Supabase user accounts
CREATE TABLE IF NOT EXISTS devices (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id TEXT UNIQUE NOT NULL,  -- Tuya device UUID (e.g., "uuid2395651a4cae9262")
    user_id UUID NOT NULL REFERENCES auth.users(id) ON DELETE CASCADE,
    name TEXT DEFAULT 'PracticePod',
    created_at TIMESTAMPTZ DEFAULT NOW(),
    last_upload_at TIMESTAMPTZ
);

-- Index for fast device lookups (critical for firmware upload path)
CREATE INDEX IF NOT EXISTS idx_devices_device_id ON devices(device_id);
CREATE INDEX IF NOT EXISTS idx_devices_user_id ON devices(user_id);

-- Enable Row Level Security
ALTER TABLE devices ENABLE ROW LEVEL SECURITY;

-- RLS Policies: Users can only manage their own devices
CREATE POLICY "Users can view their own devices"
    ON devices FOR SELECT
    USING (auth.uid() = user_id);

CREATE POLICY "Users can link devices to themselves"
    ON devices FOR INSERT
    WITH CHECK (auth.uid() = user_id);

CREATE POLICY "Users can update their own devices"
    ON devices FOR UPDATE
    USING (auth.uid() = user_id);

CREATE POLICY "Users can unlink their own devices"
    ON devices FOR DELETE
    USING (auth.uid() = user_id);

-- Allow service role to lookup devices (for firmware upload endpoint)
-- This policy allows the backend (using service_role key) to query any device
CREATE POLICY "Service role can view all devices"
    ON devices FOR SELECT
    TO service_role
    USING (true);

CREATE POLICY "Service role can update all devices"
    ON devices FOR UPDATE
    TO service_role
    USING (true);

-- Comment explaining the table purpose
COMMENT ON TABLE devices IS 'Links PracticePod firmware devices (Tuya UUIDs) to Supabase user accounts';
COMMENT ON COLUMN devices.device_id IS 'Tuya device UUID sent by firmware in X-User-ID header';
COMMENT ON COLUMN devices.user_id IS 'Supabase auth.users UUID of the device owner';
COMMENT ON COLUMN devices.last_upload_at IS 'Timestamp of most recent recording upload from this device';

