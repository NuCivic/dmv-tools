Rayogram Stress Test Tool

Description
===========

a suite of tools that allows a drupal7 site to be tested by clearing caches and publishing/unpublishing of nodes.

Drush Commands:

drush export-publish-status <nodeIDs|nodeType> <filename>
drush import-publish-status <filename>

drush stress-cache <nodeIDs|nodeType> <durationInSeconds> <durationBetweenCallsInMilliseconds>
drush stress-publish <nodeIDs|nodeType> <durationInSeconds> <durationBetweenCallsInMilliseconds>

Notes:

It is imperative to back up the drupal database before running tests since nodes are altered and the site is being stress tested which may cause it to fail.

For the command stress-publish, nodes are automatically reset at the end of the stress test. However, it is advisable to run export-publish-status in case the command does not complete. In that case import-publish-status can be used to reload the original node statuses.

Safe steps:

1) backup database
2) export node statuses
3) run stress test
4) import node statuses
5) check site, and potentially reload from db backup if there are any issues.

examples:

drush export-publish-status blog_post myNodeStatusBackup.txt
	export the status of all blog_post nodes and save to file public://myNodeStatusBackup.txt

drush stress-cache blog_post 10 200
	bust the cache for all nodes of type blog_post for 10 seconds, with 200ms interval

drush stress-publish 20,200-300 10 200
	publish/unpublish nodes 20 as well as 200 through 300 for 10 seconds, with 200ms interval

drush import-publish-status myNodeStatusBackup.txt
	reset the statuses for nodes as per file public://myNodeStatusBackup.txt

