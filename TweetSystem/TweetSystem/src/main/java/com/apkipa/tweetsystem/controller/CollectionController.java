package com.apkipa.tweetsystem.controller;

import cn.dev33.satoken.stp.StpUtil;
import com.apkipa.tweetsystem.model.*;
import com.apkipa.tweetsystem.repository.CollectionRepository;
import com.apkipa.tweetsystem.repository.PostRepository;
import com.apkipa.tweetsystem.repository.UserRepository;
import com.apkipa.tweetsystem.result.JResp;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.sql.Timestamp;
import java.time.Instant;
import java.util.Objects;

@RestController
@RequestMapping("/api/v1/collection")
public class CollectionController {
    @Autowired
    PostRepository postRepository;
    @Autowired
    UserRepository userRepository;
    @Autowired
    CollectionRepository collectionRepository;

    @GetMapping("/list")
    public JResp<?> list() {
        var collections = collectionRepository.findAllByUserId(StpUtil.getLoginIdAsLong(),
                CollectionFetcher.$
                        .allScalarFields()
                        .user()
                        .post(PostFetcher.$
                                .allScalarFields()
                                .user(UserFetcher.$.name().nickname())
                                .likes(LikeFetcher.$.user(UserFetcher.$))
                                .collections(CollectionFetcher.$.user())
                                .replyPosts()
                        )
        )
                .stream()
                .map(v -> v.post())
                .filter(Objects::nonNull)
                .toList();
        return JResp.ok(collections);
    }

    @PostMapping("/add")
    public JResp<?> add(@RequestParam Long id) {
        var userId = StpUtil.getLoginIdAsLong();
        if (collectionRepository.findByUserIdAndPostId(userId, id).isPresent()) {
            return JResp.err(-1, "不能重复收藏");
        }
        var user = userRepository.findNullable(userId);
        var postOpt = postRepository.findById(id);
        if (postOpt.isEmpty()) {
            return JResp.err(-1, "推文无效");
        }
        var post = postOpt.get();
        var collection = CollectionDraft.$.produce(draft -> {
            draft.setUser(user);
            draft.setPost(post);
            draft.setTime(Timestamp.from(Instant.now()));
        });
        collectionRepository.save(collection);
        return JResp.ok(null);
    }

    @PostMapping("/remove")
    public JResp<?> remove(@RequestParam Long id) {
        var userId = StpUtil.getLoginIdAsLong();
        var collectionOpt = collectionRepository.findByUserIdAndPostId(userId, id);
        if (collectionOpt.isEmpty()) {
            return JResp.err(-1, "尚未收藏该推文");
        }
        collectionRepository.delete(collectionOpt.get());
        return JResp.ok(null);
    }
}
