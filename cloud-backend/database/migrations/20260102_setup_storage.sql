-- Setup storage bucket for avatars
-- Run this in your Supabase SQL Editor

-- Create the bucket
INSERT INTO storage.buckets (id, name, public)
VALUES ('avatars', 'avatars', true)
ON CONFLICT (id) DO NOTHING;

-- Policy to allow public to view avatars
CREATE POLICY "Public Avatar View"
ON storage.objects FOR SELECT
USING (bucket_id = 'avatars');

-- Policy to allow users to upload their own avatar
-- This matches the filename format: user_id_timestamp.ext
CREATE POLICY "Users can upload their own avatars"
ON storage.objects FOR INSERT
TO authenticated
WITH CHECK (
  bucket_id = 'avatars' AND
  (SELECT auth.uid())::text = SPLIT_PART(name, '_', 1)
);

-- Policy to allow users to update their own avatar
CREATE POLICY "Users can update their own avatars"
ON storage.objects FOR UPDATE
TO authenticated
USING (
  bucket_id = 'avatars' AND
  (SELECT auth.uid())::text = SPLIT_PART(name, '_', 1)
);

-- Policy to allow users to delete their own avatar
CREATE POLICY "Users can delete their own avatars"
ON storage.objects FOR DELETE
TO authenticated
USING (
  bucket_id = 'avatars' AND
  (SELECT auth.uid())::text = SPLIT_PART(name, '_', 1)
);
