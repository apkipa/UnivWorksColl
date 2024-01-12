insert into user (id, role, name, nickname, password, reg_time, sex, age, email, introduction)
values
    -- admin1, root
    (1, 'ROLE_ADMIN', 'admin1', 'admin1', '4813494d137e1631bba301d5acab6e7bb7aa74ce1185d456565ef51d737677b2', '2010-12-31 01:15:00', '男', 32, 'admin@example.com', '我是管理员1'),
    -- review1, reviewer
    (2, 'ROLE_REVIEW', 'review1', 'reviewer1', '2d70999ae1805e4bcef9b4ab3a4b827f578c61740f30076fcdc35c7ae7f586b3', '2018-3-21 01:15:00', '女', 22, 'review1@example.com', '我是审核员1'),
    -- user1, user
    (3, 'ROLE_USER', 'user1', '普通用户1', '04f8996da763b7a969b1028ee3007569eaf3a635486ddab211d512c85b9df8fb', '2022-1-3 08:15:00', '男', 18, 'user@example.com', '一个普普通通的用户')
;

insert into post (id, deleted, user_id, publish_time, content, audit_state, reply_post_id, forward_post_id)
values
    -- post1
    (1, false, 1, '2023-12-10 09:15:00', '第一篇推文!', 'Passed', null, null),
    (2, false, 1, '2023-12-10 09:17:03', '第二篇推文!!', 'Passed', null, null),
    (3, false, 1, '2023-12-10 09:17:04', '第3篇推文!!', 'Passed', null, null),
    (4, false, 1, '2023-12-10 09:17:05', '第4篇推文!!', 'Passed', null, null),
    (5, false, 1, '2023-12-10 09:17:06', '第5篇推文!!', 'Passed', null, null),
    (6, false, 3, '2023-12-10 09:17:07', '第3-1篇推文!!', 'Passed', null, null),
    (7, false, 3, '2023-12-10 09:17:08', '第3-2篇推文!!', 'Passed', null, null),
    (8, false, 3, '2023-12-10 09:17:09', '第3-3篇推文!!', 'Passed', null, null),
    (9, false, 3, '2023-12-10 09:17:10', '第3-4篇推文!!', 'Passed', null, null),
    (10, false, 3, '2023-12-10 09:17:11', '第3-5篇推文!!', 'Passed', null, null)
;
