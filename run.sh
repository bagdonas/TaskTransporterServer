killall -9 ttserver
./ttserver \
--server_port 4300 \
--db_host "localhost" \
--db_port 3306 \
--db_name "task_transporter" \
--db_user "task_transporter" \
--db_password "" \
--db_task_table "task" \
--db_user_table "user" \
--db_userid_field "id" \
--db_session_field "session_hash" \
--db_connected_field "is_connected" \
--console \
--debug 