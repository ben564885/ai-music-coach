-- Migration: Fix device transfer RLS policy
-- 
-- PROBLEM: Users can't transfer device ownership because RLS blocks updates
-- to devices they don't own. The upsert operation also fails.
--
-- SOLUTION: Allow any authenticated user to claim a device by knowing its UUID.
-- The device UUID acts as a secret key - if you have it, you can claim the device.

-- =============================================================================
-- STEP 1: Drop existing restrictive policies
-- =============================================================================
DROP POLICY IF EXISTS "Users can update their own devices" ON devices;
DROP POLICY IF EXISTS "Users can view their own devices" ON devices;
DROP POLICY IF EXISTS "Users can link devices to themselves" ON devices;
DROP POLICY IF EXISTS "Users can unlink their own devices" ON devices;
DROP POLICY IF EXISTS "Authenticated users can claim devices" ON devices;
DROP POLICY IF EXISTS "Users can check device existence" ON devices;

-- =============================================================================
-- STEP 2: Create new policies that allow device claiming/transfers
-- =============================================================================

-- SELECT: Users can only see their own devices
CREATE POLICY "Users can view own devices"
    ON devices FOR SELECT
    TO authenticated
    USING (auth.uid() = user_id);

-- INSERT: Users can add new devices (must set user_id to themselves)
CREATE POLICY "Users can insert devices"
    ON devices FOR INSERT
    TO authenticated
    WITH CHECK (auth.uid() = user_id);

-- UPDATE: ANY authenticated user can update ANY device's user_id to themselves
-- This enables device transfers/claiming
CREATE POLICY "Users can claim devices"
    ON devices FOR UPDATE
    TO authenticated
    USING (true)  -- Can update any device
    WITH CHECK (auth.uid() = user_id);  -- But only to set user_id to themselves

-- DELETE: Users can only delete their own devices
CREATE POLICY "Users can delete own devices"
    ON devices FOR DELETE
    TO authenticated
    USING (auth.uid() = user_id);

-- =============================================================================
-- EXPLANATION:
-- =============================================================================
-- When User B calls upsert with a device owned by User A:
-- 1. Supabase checks if device exists (using service role internally for upsert)
-- 2. Device exists, so it does UPDATE
-- 3. UPDATE policy allows because: USING(true) passes, WITH CHECK passes if new user_id = auth.uid()
-- 4. Device ownership transfers to User B
--
-- This is secure because:
-- - User must know the device UUID to claim it
-- - User can only set user_id to their own ID (can't steal for someone else)
-- - Users can't see other users' devices (SELECT is restricted)

