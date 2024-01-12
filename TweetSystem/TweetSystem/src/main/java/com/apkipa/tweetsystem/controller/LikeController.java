package com.apkipa.tweetsystem.controller;

import cn.dev33.satoken.stp.StpUtil;
import com.apkipa.tweetsystem.model.LikeDraft;
import com.apkipa.tweetsystem.repository.LikeRepository;
import com.apkipa.tweetsystem.repository.PostRepository;
import com.apkipa.tweetsystem.repository.UserRepository;
import com.apkipa.tweetsystem.result.JResp;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.sql.Timestamp;
import java.time.Instant;

@RestController
@RequestMapping("/api/v1/like")
public class LikeController {
    @Autowired
    PostRepository postRepository;
    @Autowired
    UserRepository userRepository;
    @Autowired
    LikeRepository likeRepository;

    @PostMapping("/add")
    public JResp<?> add(@RequestParam Long id) {
        var userId = StpUtil.getLoginIdAsLong();
        if (likeRepository.findByUserIdAndPostId(userId, id).isPresent()) {
            return JResp.err(-1, "不能重复点赞");
        }
        var user = userRepository.findNullable(userId);
        var postOpt = postRepository.findById(id);
        if (postOpt.isEmpty()) {
            return JResp.err(-1, "推文无效");
        }
        var post = postOpt.get();
        var like = LikeDraft.$.produce(draft -> {
            draft.setUser(user);
            draft.setPost(post);
            draft.setTime(Timestamp.from(Instant.now()));
        });
        likeRepository.save(like);
        return JResp.ok(null);
    }

    @PostMapping("/remove")
    public JResp<?> remove(@RequestParam Long id) {
        var userId = StpUtil.getLoginIdAsLong();
        var likeOpt = likeRepository.findByUserIdAndPostId(userId, id);
        if (likeOpt.isEmpty()) {
            return JResp.err(-1, "尚未点赞此推文");
        }
        likeRepository.delete(likeOpt.get());
        return JResp.ok(null);
    }
}
